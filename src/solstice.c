/* Copyright (C) CNRS 2016-2017
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>. */

#include "solstice.h"
#include "solstice_c.h"
#include "solstice_args.h"
#include "parser/solparser.h"

#include <solstice/ssol.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
clear_materials(struct htable_material* materials)
{
  struct htable_material_iterator it, end;
  ASSERT(materials);

  htable_material_begin(materials, &it);
  htable_material_end(materials, &end);
  while(!htable_material_iterator_eq(&it, &end)) {
    struct ssol_material* mtl = *htable_material_iterator_data_get(&it);
    SSOL(material_ref_put(mtl));
    htable_material_iterator_next(&it);
  }
  htable_material_clear(materials);
}

static void
clear_objects(struct htable_object* objects)
{
  struct htable_object_iterator it, end;
  ASSERT(objects);

  htable_object_begin(objects, &it);
  htable_object_end(objects, &end);
  while(!htable_object_iterator_eq(&it, &end)) {
    struct ssol_object* obj = *htable_object_iterator_data_get(&it);
    SSOL(object_ref_put(obj));
    htable_object_iterator_next(&it);
  }
  htable_object_clear(objects);
}

static void
clear_nodes(struct darray_nodes* nodes)
{
  size_t i, n;
  ASSERT(nodes);
  n = darray_nodes_size_get(nodes);
  FOR_EACH(i, 0, n) {
    solstice_node_ref_put(darray_nodes_data_get(nodes)[i]);
  }
  darray_nodes_clear(nodes);
}

static res_T
setup_camera(struct solstice* solstice, const struct solstice_args* args)
{
  struct ssol_camera* cam = NULL;
  double proj_ratio = 0;
  res_T res = RES_OK;
  ASSERT(solstice && args);

  res = ssol_camera_create(solstice->ssol, &cam);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the rendering camera.\n");
    goto error;
  }

  proj_ratio = (double)args->img.width / (double)args->img.height;
  res = ssol_camera_set_proj_ratio(cam, proj_ratio);
  if(res != RES_OK) {
    fprintf(stderr, "Invalid image ratio '%g'.\n", proj_ratio);
    goto error;
  }

  res = ssol_camera_set_fov(cam, args->camera.fov_x);
  if(res != RES_OK) {
    fprintf(stderr,
      "Invalid horizontal field of view '%g degrees' (%g radians).\n",
      MRAD2DEG(args->camera.fov_x), args->camera.fov_x);
    goto error;
  }

  res = ssol_camera_look_at
    (cam, args->camera.pos, args->camera.tgt, args->camera.up);
  if(res != RES_OK) {
    fprintf(stderr,
"Invalid camera point of view:\n"
"  position = %g %g %g\n"
"  target = %g %g %g\n"
"  up = %g %g %g\n",
      SPLIT3(args->camera.pos),
      SPLIT3(args->camera.tgt),
      SPLIT3(args->camera.up));
    goto error;
  }

exit:
  solstice->camera = cam;
  return res;
error:
  if(cam) {
    SSOL(camera_ref_put(cam));
    cam = NULL;
  }
  goto exit;
}

static res_T
setup_framebuffer(struct solstice* solstice, const struct solstice_args* args)
{
  struct ssol_image* fbuf = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && args);

  res = ssol_image_create(solstice->ssol, &fbuf);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the rendering framebuffer.\n");
    goto error;
  }

  res = ssol_image_setup
    (fbuf, args->img.width, args->img.height, SSOL_PIXEL_DOUBLE3);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not set the framebuffer definition to %lux%lu.\n",
      args->img.width, args->img.height);
    goto error;
  }

exit:
  solstice->framebuffer = fbuf;
  return res;

error:
  if(fbuf) {
    SSOL(image_ref_put(fbuf));
    fbuf = NULL;
  }
  goto exit;
}

static INLINE void
spherical_to_cartesian_sun_dir
  (struct solstice_args_spherical* spherical, double sun_dir[3])
{
  double cos_azimuth;
  double sin_azimuth;
  double cos_elevation;
  double sin_elevation;
  ASSERT(spherical && sun_dir);

  cos_azimuth = cos(spherical->azimuth);
  sin_azimuth = sin(spherical->azimuth);
  cos_elevation = cos(spherical->elevation);
  sin_elevation = sin(spherical->elevation);

  sun_dir[0] = -(cos_elevation * cos_azimuth);
  sun_dir[1] = -(cos_elevation * sin_azimuth);
  sun_dir[2] = -(sin_elevation);
}

static res_T
setup_sun_dirs(struct solstice* solstice, const struct solstice_args* args)
{
  double* sun_dirs = NULL;
  size_t i;
  res_T res = RES_OK;
  ASSERT(solstice && args);

  res = darray_double_resize(&solstice->sun_dirs, args->nsun_dirs*3/*#dims*/);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not reserve the list of %lu sun directions.\n", args->nsun_dirs);
    goto error;

  }

  sun_dirs = darray_double_data_get(&solstice->sun_dirs);
  FOR_EACH(i, 0, args->nsun_dirs) {
    spherical_to_cartesian_sun_dir(args->sun_dirs + i, sun_dirs + i*3/*#dims*/);
  }

exit:
  return res;
error:
  darray_double_clear(&solstice->sun_dirs);
  goto exit;
}

static res_T
load_data(struct solstice* solstice, const struct solstice_args* args)
{
  FILE* file = stdin;
  const char* name = "stdin";
  res_T res = RES_OK;
  ASSERT(solstice && args);

  if(args->input_filename) {
    file = fopen(args->input_filename, "r");
    if(!file) {
      fprintf(stderr, "Could not open the file `%s'.\n", args->input_filename);
      res = RES_IO_ERR;
      goto error;
    }
    name = args->input_filename;
  } else if(!args->quiet) {
#ifndef OS_WINDOWS
    fprintf(stderr,
      "Enter the solar facility data. Type ^D (i.e. CTRL+d) to stop:\n");
#else
    fprintf(stderr,
      "Enter the solar facility data. Type ^Z (i.e. CTRL+z) to stop:\n");
#endif
  }

  res = solparser_setup(solstice->parser, name, file);
  if(res != RES_OK) goto error;

  res = solparser_load(solstice->parser);
  if(res != RES_OK) goto error;

  res = solstice_setup_entities(solstice);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the Solstice entities.\n");
    goto error;
  }

exit:
  if(file && file != stdin) fclose(file);
  return res;
error:
  goto exit;
}

static res_T
parse_receiver_file(struct solstice* solstice, const char* filename)
{
  char line[256];
  FILE* file = NULL;
  unsigned long iline = 0;
  res_T res = RES_OK;
  ASSERT(solstice && filename);

  file = fopen(filename, "r");
  if(!file) {
    fprintf(stderr, "Could not open the file `%s'.\n", filename);
    res = RES_IO_ERR;
    goto error;
  }

  while(fgets(line, sizeof(line)/sizeof(char), file)) {
    const struct solparser_entity* entity;
    struct solstice_node* node = NULL;
    char* str;
    char* p;
    size_t last_char;
    size_t first_char;

    ++iline;

    if(!strrchr(line, '\n')) {
      fprintf(stderr,
        "%s:%lu: insufficient memory. Could not parse the receiver name.\n",
        filename, iline);
      return RES_MEM_ERR;
    }

    /* Remove the new line character(s) */
    last_char = strlen(line);
    while(last_char-- && (line[last_char] == '\n' || line[last_char]=='\r'));
    line[last_char+1] = '\0';

    /* Strip beginning white spaces */
    first_char = 0;
    while(first_char < last_char
       && (line[first_char]==' ' || line[first_char]=='\t')) {
      ++first_char;
    }
    str = line + first_char;

    /* Discard comments */
    if((p = strchr(str, '#'))) *p = '\0';

    /* Strip ending white spaces */
    last_char = strlen(str);
    while(last_char-- && (line[last_char] == ' ' || line[last_char] == '\t'));
    str[last_char+1] = '\0';

    /* Discard empty lines */
    if(str[0] == '\0') continue;

    /* Retrieve the entity */
    entity = solparser_find_entity(solstice->parser, str);
    if(!entity) {
      fprintf(stderr, "%s:%lu: invalid entity `%s'.\n",
        filename, iline, str);
    }

    /* Register the entity as a receiver */
    res = htable_entity_set(&solstice->receivers, &entity, &node);
    if(res != RES_OK) {
      fprintf(stderr,
        "%s:%lu could not register the entity `%s' as a receiver.\n",
        filename, iline, str);
      goto error;
    }
  }

exit:
  if(file) fclose(file);
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Solstice local functions
 ******************************************************************************/
res_T
solstice_init
  (struct mem_allocator* allocator,
   const struct solstice_args* args,
   struct solstice* solstice)
{
  res_T res = RES_OK;
  ASSERT(solstice && args);

  memset(solstice, 0, sizeof(struct solstice));
  htable_material_init(allocator, &solstice->materials);
  htable_object_init(allocator, &solstice->objects);
  htable_anchor_init(allocator, &solstice->anchors);
  htable_entity_init(allocator, &solstice->receivers);
  darray_nodes_init(allocator, &solstice->roots);
  darray_nodes_init(allocator, &solstice->pivots);
  darray_double_init(allocator, &solstice->sun_dirs);

  solstice->allocator = allocator ? allocator : &mem_default_allocator;

  res = ssol_device_create
    (NULL, allocator, SSOL_NTHREADS_DEFAULT, 0, &solstice->ssol);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver device.\n");
    goto error;
  }

  res = ssol_scene_create(solstice->ssol, &solstice->scene);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver scene.\n");
    goto error;
  }

  res = solparser_create(allocator, &solstice->parser);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Parser.\n");
    goto error;
  }

  if(args->rendering) {
    res = setup_camera(solstice, args);
    if(res != RES_OK) goto error;
    res = setup_framebuffer(solstice, args);
    if(res != RES_OK) goto error;
  }

  res = setup_sun_dirs(solstice, args);
  if(res != RES_OK) goto error;

  if(!args->output_filename) {
    solstice->output = stdout;
  } else {
    solstice->output = fopen(args->output_filename, "w+");
    if(!solstice->output) {
      fprintf(stderr, "Could not open the output file `%s'.\n",
        args->output_filename);
      res = RES_IO_ERR;
      goto error;
    }
  }

  res = load_data(solstice, args);
  if(res != RES_OK) goto error;

exit:
  return res;
error:
  solstice_release(solstice);
  goto exit;
}

void
solstice_release(struct solstice* solstice)
{
  ASSERT(solstice);
  clear_materials(&solstice->materials);
  clear_objects(&solstice->objects);
  clear_nodes(&solstice->roots);
  /* don't clear pivots, as they are cleared from some root */
  if(solstice->ssol) SSOL(device_ref_put(solstice->ssol));
  if(solstice->scene) SSOL(scene_ref_put(solstice->scene));
  if(solstice->parser) solparser_ref_put(solstice->parser);
  if(solstice->camera) SSOL(camera_ref_put(solstice->camera));
  if(solstice->framebuffer) SSOL(image_ref_put(solstice->framebuffer));
  if(solstice->output && solstice->output != stdout) fclose(solstice->output);
  htable_material_release(&solstice->materials);
  htable_object_release(&solstice->objects);
  htable_anchor_release(&solstice->anchors);
  htable_entity_release(&solstice->receivers);
  darray_nodes_release(&solstice->roots);
  darray_nodes_release(&solstice->pivots);
  darray_double_release(&solstice->sun_dirs);
}

res_T
solstice_run(struct solstice* solstice)
{
  const double* sun_dirs = NULL;
  size_t nsun_dirs = 0;
  size_t i;
  res_T res = RES_OK;
  ASSERT(solstice);

  sun_dirs = darray_double_cdata_get(&solstice->sun_dirs);
  nsun_dirs = darray_double_size_get(&solstice->sun_dirs);
  ASSERT(nsun_dirs%3 == 0);
  nsun_dirs /= 3/*#dims*/;

  if(!solstice->framebuffer) { /* Solstice integration */
    res = RES_BAD_OP; /* TODO */
    goto error;
  } else if(!nsun_dirs) {
    res = solstice_draw(solstice);
    if(res != RES_OK) goto error;
  } else {
    FOR_EACH(i, 0, nsun_dirs) {
      const double* sun_dir = sun_dirs + i*3/*#dims*/;

      res = solstice_update_entities(solstice, sun_dir);
      if(res != RES_OK) goto error;

      res = solstice_draw(solstice);
      if(res != RES_OK) goto error;

      fprintf(solstice->output,
        "# Sun direction: %g %g %g\n", SPLIT3(sun_dir));
    }
  }

exit:
  return res;
error:
  goto exit;
}

