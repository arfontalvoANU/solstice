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

exit:
  if(file && file != stdin) fclose(file);
  return res;
error:
  goto exit;
}

static res_T
setup_receivers(struct solstice* solstice, struct srcvl* srcvl)
{
  struct solstice_receiver receiver;
  size_t i, n;
  res_T res = RES_OK;
  ASSERT(solstice && srcvl);

  solstice_receiver_init(solstice->allocator, &receiver);

  n = srcvl_count(srcvl);
  FOR_EACH(i, 0, n) {
    struct srcvl_receiver rcv;
    const struct solparser_entity* entity;

    srcvl_get(srcvl, i, &rcv);
    entity = solparser_find_entity(solstice->parser, rcv.name);
    if(!entity) {
      fprintf(stderr, "Invalid entity `%s'.\n", rcv.name);
      res = RES_BAD_ARG;
      goto error;
    }

    if(entity->type != SOLPARSER_ENTITY_GEOMETRY) {
      fprintf(stderr,
        "The entity `%s' is not a geometry. It cannot be a receiver.\n",
        rcv.name);
      res = RES_BAD_ARG;
      goto error;
    }

    receiver.node = NULL;
    receiver.side = rcv.side;
    str_set(&receiver.name, rcv.name);

    res = htable_receiver_set(&solstice->receivers, &entity, &receiver);
    if(res != RES_OK) {
      fprintf(stderr,
        "Could not register the receiver `%s' against Solstice.\n",
        rcv.name);
      goto error;
    }
  }

exit:
  solstice_receiver_release(&receiver);
  return res;
error:
  htable_receiver_clear(&solstice->receivers);
  goto exit;
}

static res_T
load_receivers(struct solstice* solstice, const struct solstice_args* args)
{
  FILE* file = NULL;
  struct srcvl* srcvl = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && args);

  file = fopen(args->receivers_filename, "r");
  if(!file) {
    fprintf(stderr, "Could not open the list of receivers `%s'.\n",
      args->receivers_filename);
    res = RES_IO_ERR;
    goto error;
  }

  res = srcvl_create(solstice->allocator, &srcvl);
  if(res != RES_OK) goto error;
  res = srcvl_setup_stream(srcvl, args->receivers_filename, file);
  if(res != RES_OK) goto error;
  res = srcvl_load(srcvl);
  if(res != RES_OK) goto error;
  res = setup_receivers(solstice, srcvl);
  if(res != RES_OK) goto error;

exit:
  if(file) fclose(file);
  if(srcvl) {
    srcvl_ref_put(srcvl);
    srcvl = NULL;
  }
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
  htable_receiver_init(allocator, &solstice->receivers);
  darray_nodes_init(allocator, &solstice->roots);
  darray_nodes_init(allocator, &solstice->pivots);
  darray_double_init(allocator, &solstice->sun_dirs);

  solstice->allocator = allocator ? allocator : &mem_default_allocator;

  res = ssol_device_create(NULL, allocator, args->nthreads, 0, &solstice->ssol);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver device.\n");
    goto error;
  }

  res = ssol_scene_create(solstice->ssol, &solstice->scene);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver scene.\n");
    goto error;
  }

  res = ssol_material_create_virtual(solstice->ssol, &solstice->mtl_virtual);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the global virtual material.\n");
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

  if(args->receivers_filename) {
    res = load_receivers(solstice, args);
    if(res != RES_OK) goto error;
  }

  res = solstice_setup_entities(solstice);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the Solstice entities.\n");
    goto error;
  }

  res = solstice_create_sun(solstice);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the Solstice sun.\n");
    goto error;
  }

  solstice->nrealisations = args->nrealisations;
  solstice->output_hits = args->output_hits;

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
  /* Don't clear pivots, as they are cleared from some root */
  if(solstice->ssol) SSOL(device_ref_put(solstice->ssol));
  if(solstice->scene) SSOL(scene_ref_put(solstice->scene));
  if(solstice->sun) SSOL(sun_ref_put(solstice->sun));
  if(solstice->parser) solparser_ref_put(solstice->parser);
  if(solstice->camera) SSOL(camera_ref_put(solstice->camera));
  if(solstice->framebuffer) SSOL(image_ref_put(solstice->framebuffer));
  if(solstice->output && solstice->output != stdout) fclose(solstice->output);
  if(solstice->mtl_virtual) SSOL(material_ref_put(solstice->mtl_virtual));
  htable_material_release(&solstice->materials);
  htable_object_release(&solstice->objects);
  htable_anchor_release(&solstice->anchors);
  htable_receiver_release(&solstice->receivers);
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
    FOR_EACH(i, 0, nsun_dirs) {
      const double* sun_dir = sun_dirs + i*3/*#dims*/;
      res = ssol_sun_set_direction(solstice->sun, sun_dir);
      if(res != RES_OK) {
        fprintf(stderr, "Could not update the sun direction.\n");
        goto error;
      }
      res = solstice_update_entities(solstice, sun_dir);
      if(res != RES_OK) goto error;
      res = solstice_solve(solstice);
      if(res != RES_OK) goto error;
    }
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

