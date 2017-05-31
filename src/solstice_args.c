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

#define _POSIX_C_SOURCE 2

#include "solstice_args.h"

#include <rsys/cstr.h>
#include <rsys/double3.h>
#include <rsys/stretchy_array.h>

#ifdef COMPILER_CL
  #include <getopt.h>
  #define strtok_r strtok_s
#else
  #include <unistd.h>
#endif

#include <string.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
print_help(const char* program)
{
  printf(
"Usage: %s [OPTIONS] [FILE]\n"
"Integrate the solar flux in a complex solar facility described in FILE. If not\n"
"define, the solar facility is read from standard input. Refer to solstice(1)\n"
"man page for more informations.\n\n",
    program);
  printf(
"  -D <dirs>        list of sun directions.\n");
  printf(
"  -f               do not prompt before overwriting the output file submitted\n"
"                   with the '-o' option.\n");
  printf(
"  -g <dump>        switch in dump geometry mode and configure it.\n");
  printf(
"  -h               display this help and exit.\n");
  printf(
"  -n EXPERIMENTS   number of Monte Carlo experiments. Default is %lu.\n",
    SOLSTICE_ARGS_DEFAULT.nexperiments);
  printf(
"  -o OUTPUT        write results to OUTPUT. If not defined, write results to\n"
"                   standard output.\n");
  printf(
"  -p <dump-paths>  switch in dump radiative paths mode and configure it.\n");
  printf(
"  -q               do not print the helper message when no FILE is submitted.\n");
  printf(
"  -R RECEIVERS     define the file from which the list of receivers are read.\n");
  printf(
"  -r <rendering>   switch in rendering mode and configure it.\n");
  printf(
"  -t THREADS       hint on the number of threads to use. By default use as\n"
"                   many threads as CPU cores.\n");
  printf(
"  -v               make the program more verbose.\n");
  printf("\n");
  printf(
"Solstice (C) 2016-2017 CNRS. This is a free software released under the GNU GPL\n"
"license, version 3 or later. You are free to change or redistribute it under\n"
"certain conditions <http://gnu.org/licenses/gpl.html>.\n");
}

static res_T
parse_fov(const char* str, double* out_fov)
{
  double fov;
  res_T res = RES_OK;
  ASSERT(str && out_fov);

  res = cstr_to_double(str, &fov);
  if(res != RES_OK) {
    fprintf(stderr, "Invalid field of view `%s'.\n", str);
    return RES_BAD_ARG;
  }

  if(fov < 30 || fov > 120) {
    fprintf(stderr, "The field of view %g is not in [30, 120].\n", fov);
    return RES_BAD_ARG;
  }
  *out_fov = fov;
  return RES_OK;
}

static res_T
parse_multiple_options
  (const char* str,
   struct solstice_args* args,
   res_T (*parse_option)(const char* str, struct solstice_args* args))
{
  char buf[512];
  char* tk;
  char* ctx;
  res_T res = RES_OK;
  ASSERT(args && str);

  if(strlen(str) >= sizeof(buf) - 1/*NULL char*/) {
    fprintf(stderr, "Could not duplicate the option string `%s'.\n", str);
    res = RES_MEM_ERR;
    goto error;
  }
  strncpy(buf, str, sizeof(buf));

  tk = strtok_r(buf, ":", &ctx);
  do {
    res = parse_option(tk, args);
    if(res != RES_OK) goto error;
    tk = strtok_r(NULL, ":", &ctx);
  } while(tk);

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_sun_dir_list(const char* str, struct solstice_args* args)
{
  char buf[512];
  char* tk;
  char* ctx;
  size_t len;
  res_T res = RES_OK;
  ASSERT(str && args);

  if(strlen(str) >= sizeof(buf) - 1/*NULL char*/) {
    fprintf(stderr,
      "Could not duplicate the list of sun directions `%s'.\n", str);
    res = RES_MEM_ERR;
    goto error;
  }
  strncpy(buf, str, sizeof(buf));

  tk = strtok_r(buf, ":", &ctx);
  while(tk) {
    struct solstice_args_spherical spherical;
    double tmp[2];

    res = cstr_to_list_double(tk, ',', tmp, &len, 2);
    if(res == RES_OK && len != 2) res = RES_BAD_ARG;
    if(res != RES_OK) {
      fprintf(stderr, "Invalid sun direction `%s'.\n", tk);
      goto error;
    }

    if(tmp[0] < 0 || tmp[0] >= 360) {
      fprintf(stderr,
        "Invalid azimuth angle `%g'. Azimuth must be in [0, 360[ degrees.\n",
        tmp[0]);
      res = RES_BAD_ARG;
      goto error;
    }
    if(tmp[1] < 0 || tmp[1] > 90) {
      fprintf(stderr,
        "Invalid elevation angle `%g'. Elevation must be in [0, 90] degrees.\n",
        tmp[1]);
      res = RES_BAD_ARG;
      goto error;
    }

    spherical.azimuth = tmp[0];
    spherical.elevation = tmp[1];
    sa_push(args->sun_dirs, spherical);

    tk = strtok_r(NULL, ":", &ctx);
  }

  args->nsun_dirs += sa_size(args->sun_dirs);

exit:
  return res;
error:
  if(args->sun_dirs) {
    sa_release(args->sun_dirs);
    args->sun_dirs = NULL;
    args->nsun_dirs = 0;
  }
  goto exit;
}

static res_T
parse_image_definition
  (const char* str,
   unsigned long* width,
   unsigned long* height)
{
  char buf[64];
  char* tk;
  char* ctx;
  res_T res = RES_OK;
  ASSERT(str && width && height);

  if(strlen(str) >= sizeof(buf) - 1/*NULL char*/) {
    fprintf(stderr,
      "Could not duplicate the image definition string `%s'.\n", str);
    return RES_MEM_ERR;
  }
  strncpy(buf, str, sizeof(buf));

  tk = strtok_r(buf, "x", &ctx);
  res = cstr_to_ulong(tk, width);
  if(res == RES_OK && !*width) res = RES_BAD_ARG;
  if(res != RES_OK) {
    fprintf(stderr, "Invalid image width `%s'\n", tk);
    return res;
  }

  tk = strtok_r(NULL, "", &ctx);
  res = cstr_to_ulong(tk, height);
  if(res == RES_OK && !*height) res = RES_BAD_ARG;
  if(res != RES_OK) {
    fprintf(stderr, "Invalid image height `%s'\n", tk);
    return res;
  }

  return res;
}

static res_T
parse_render_mode(const char* str, enum solstice_args_render_mode* mode)
{
  res_T res = RES_OK;
  ASSERT(str && mode);

  if(!strcmp(str, "draft")) {
    *mode = SOLSTICE_ARGS_RENDER_DRAFT;
  } else if(!strcmp(str, "pt")) {
    *mode = SOLSTICE_ARGS_RENDER_PATH_TRACING;
  } else {
    fprintf(stderr, "Invalid render mode `%s'.\n", str);
    res = RES_BAD_ARG;
    goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_rendering_option(const char* str, struct solstice_args* args)
{
  char buf[128];
  char* key;
  char* val;
  char* ctx;
  size_t len;
  res_T res;
  ASSERT(str && args);

  if(strlen(str) >= sizeof(buf) - 1/*NULL char*/) {
    fprintf(stderr,
      "Could not duplicate the rendering option string `%s'\n", str);
    res = RES_MEM_ERR;
    goto error;
  }
  strncpy(buf, str, sizeof(buf));

  key = strtok_r(buf, "=", &ctx);
  val = strtok_r(NULL, "", &ctx);

  if(!val) {
    fprintf(stderr, "Missing a value to the rendering option `%s'.\n", key);
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp(key, "fov")) { /* Camera horizontal field of view in degrees */
    res = parse_fov(val, &args->camera.fov_x);
    if(res != RES_OK) goto error;
  } else if(!strcmp(key, "img")) { /* Image definition */
    res = parse_image_definition(val, &args->img.width, &args->img.height);
    if(res != RES_OK) goto error;
  } else if(!strcmp(key, "pos")) { /* Camera position */
    res = cstr_to_list_double(val, ',', args->camera.pos, &len, 3);
    if(res == RES_OK && len != 3) res = RES_BAD_ARG;
    if(res != RES_OK ) {
      fprintf(stderr, "Invalid camera position `%s'.\n", val);
      goto error;
    }
    args->camera.auto_look_at = 0; /* Disable auto look at */
  } else if(!strcmp(key, "rmode")) { /* Render mode */
    res = parse_render_mode(val, &args->render_mode);
    if(res != RES_OK) goto error;
  } else if(!strcmp(key, "spp")) { /*# Samples per pixel */
    res = cstr_to_uint(val, &args->img.spp);
    if(res == RES_OK && !args->img.spp) res = RES_BAD_ARG;
    if(res != RES_OK) {
      fprintf(stderr, "Invalid number of samples per pixel `%s'.\n", val);
      goto error;
    }
  } else if(!strcmp(key, "tgt")) { /* Camera target */
    res = cstr_to_list_double(val, ',', args->camera.tgt, &len, 3);
    if(res == RES_OK && len != 3) res = RES_BAD_ARG;
    if(res != RES_OK) {
      fprintf(stderr, "Invalid camera target `%s'.\n", val);
      goto error;
    }
    args->camera.auto_look_at = 0; /* Disable auto look at */
  } else if(!strcmp(key, "up")) { /* Camera up vector */
    res = cstr_to_list_double(val, ',', args->camera.up, &len, 3);
    if(res == RES_OK && len != 3) res = RES_BAD_ARG;
    if(res != RES_OK) {
      fprintf(stderr, "Invalid camera up vector `%s'.\n", val);
      goto error;
    }
  } else {
    fprintf(stderr, "Invalid rendering option `%s'.\n", val);
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_dump_format(const char* str, enum solstice_args_dump_format* fmt)
{
  res_T res = RES_OK;
  ASSERT(str && fmt);

  if(!strcmp(str, "obj")) {
    *fmt = SOLSTICE_ARGS_DUMP_OBJ;
  } else {
    fprintf(stderr, "Invalid dump format `%s'.\n", str);
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_dump_split_mode(const char* str, enum solstice_args_dump_split_mode* mode)
{
  res_T res = RES_OK;
  ASSERT(str && mode);

  if(!strcmp(str, "geometry")) {
    *mode = SOLSTICE_ARGS_DUMP_SPLIT_GEOMETRY;
  } else if(!strcmp(str, "none")) {
    *mode = SOLSTICE_ARGS_DUMP_SPLIT_NONE;
  } else if(!strcmp(str, "object")) {
    *mode = SOLSTICE_ARGS_DUMP_SPLIT_OBJECT;
  } else {
    fprintf(stderr, "Invalid dump split mode `%s'.\n", str);
    res = RES_BAD_ARG;
    goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_dump_option(const char* str, struct solstice_args* args)
{
  char buf[128];
  char* key;
  char* val;
  char* ctx;
  res_T res = RES_OK;
  ASSERT(str && args);

  if(strlen(str) >= sizeof(buf) - 1/*NULL char*/) {
    fprintf(stderr,
      "Could not duplicate the dump geometry option string `%s'\n", str);
    res = RES_MEM_ERR;
    goto error;
  }
  strncpy(buf, str, sizeof(buf));

  key = strtok_r(buf, "=", &ctx);
  val = strtok_r(NULL, "", &ctx);

  if(!val) {
    fprintf(stderr, "Missing a value to the dump option `%s'.\n", key);
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp(key, "format")) {
    res = parse_dump_format(val, &args->dump_format);
  } else if(!strcmp(key, "split")) {
    res = parse_dump_split_mode(val, &args->dump_split_mode);
  } else {
    fprintf(stderr, "Invalid dump option `%s'.\n", val);
    res = RES_BAD_ARG;
    goto error;
  }
  if(res != RES_OK) goto error;

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_dump_paths_option(const char* str, struct solstice_args* args)
{
  char buf[128];
  char* key;
  char* val;
  char* ctx;
  res_T res = RES_OK;
  ASSERT(str && args);

  if(!strcmp(str, "default")) {
    args->infinite_ray_length = SOLSTICE_ARGS_DEFAULT.infinite_ray_length;
    args->sun_ray_length = SOLSTICE_ARGS_DEFAULT.sun_ray_length;
    goto exit;
  }

  if(strlen(str) >= sizeof(buf) - 1/*NULL char*/) {
    fprintf(stderr,
"Could not duplicate the dump radiative paths option string `%s'.\n", str);
    res = RES_MEM_ERR;
    goto error;
  }

  strncpy(buf, str, sizeof(buf));

  key = strtok_r(buf, "=", &ctx);
  val = strtok_r(NULL, "", &ctx);

  if(!val) {
    fprintf(stderr,
      "Missing a value to the dump radiative paths option `%s'.\n", key);
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp(key, "irlen")) {
    res = cstr_to_double(val, &args->infinite_ray_length);
    if(res != RES_OK) {
      fprintf(stderr, "Invalid infinite ray length `%s'.\n", val);
      goto error;
    }
  } else if(!strcmp(key, "srlen")) {
    res = cstr_to_double(val, &args->sun_ray_length);
    if(res != RES_OK) {
      fprintf(stderr, "Invalid sun ray length `%s'.\n", val);
      goto error;
    }
  } else {
    fprintf(stderr, "Invalid dump radiative paths option `%s'.\n", val);
    res = RES_BAD_ARG;
    goto error;
  }
  if(res != RES_OK) goto error;

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Local function
 ******************************************************************************/
res_T
solstice_args_init(struct solstice_args* args, const int argc, char** argv)
{
  int opt;
  res_T res = RES_OK;
  ASSERT(args && argc && argv);

  *args = SOLSTICE_ARGS_DEFAULT;

  optind = 0;
  while((opt = getopt(argc, argv, "D:fg:hn:o:p:qR:r:t:v")) != -1) {
    switch(opt) {
      case 'D': /* Sun directions */
        res = parse_sun_dir_list(optarg, args);
        break;
      case 'f': args->force_overwriting = 1; break;
      case 'h': /* Print short help and exit */
        print_help(argv[0]);
        solstice_args_release(args);
        args->quit = 1;
        goto exit;
      case 'n': /* Define the number of experiments */
        res = cstr_to_ulong(optarg, &args->nexperiments);
        if(res == RES_OK && !args->nexperiments) res = RES_BAD_ARG;
        break;
      case 'g': /* Switch in dump geometry mode and configure it */
        res = parse_multiple_options(optarg, args, parse_dump_option);
        if(res == RES_OK && args->dump_format == SOLSTICE_ARGS_DUMP_NONE) {
          fprintf(stderr, "%s: missing a dump format -- `%s'.\n",
            argv[0], optarg);
          res = RES_BAD_ARG;
        }
        break;
      case 'o': args->output_filename = optarg; break;
      case 'p': /* Switch in dump radiative paths mode and configure it */
        args->dump_paths = 1;
        res = parse_multiple_options(optarg, args, parse_dump_paths_option);
        break;
      case 'q': args->quiet = 1; break;
      case 'R': args->receivers_filename = optarg; break;
      case 'r':  /* Switch in rendering mode and configure it */
        args->rendering = 1;
        res = parse_multiple_options(optarg, args, parse_rendering_option);
        break;
      case 't': /* Submit an hint on the number of threads to use */
        res = cstr_to_uint(optarg, &args->nthreads);
        if(res == RES_OK && !args->nthreads) res = RES_BAD_ARG;
        break;
      case 'v': args->verbose = 1; break;
      default: res = RES_BAD_ARG; break;
    }
    if(res != RES_OK) {
      if(optarg) {
        fprintf(stderr, "%s: invalid option argument '%s' -- '%c'\n",
          argv[0], optarg, opt);
      }
      goto error;
    }
  }

  if(!args->rendering
  && args->dump_format == SOLSTICE_ARGS_DUMP_NONE
  && !args->nsun_dirs) {
    fprintf(stderr, "Missing sun direction.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(args->dump_format != SOLSTICE_ARGS_DUMP_NONE && args->rendering) {
    fprintf(stderr, "The '-g' and '-r' options are exclusives.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(args->dump_format != SOLSTICE_ARGS_DUMP_NONE && args->dump_paths) {
    fprintf(stderr, "The '-g' and '-p' options are exclusives.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(args->dump_paths && args->rendering) {
    fprintf(stderr, "The '-p' and '-r' options are exclusives.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(optind < argc) {
    args->input_filename = argv[optind];
  }

exit:
  optind = 1;
  return res;
error:
  solstice_args_release(args);
  goto exit;
}

void
solstice_args_release(struct solstice_args* args)
{
  ASSERT(args);
  sa_release(args->sun_dirs);
  *args = SOLSTICE_ARGS_NULL;
}

