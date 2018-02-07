/* Copyright (C) CNRS 2016-2018
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

#include "solstice_args.h"
#include "test_solstice_utils.h"

#include <rsys/double3.h>
#include <rsys/stretchy_array.h>

#include <stdarg.h>
#include <string.h>
#include <limits.h>

#ifdef COMPILER_CL
  #pragma warning(push)
  #pragma warning(disable:4706) /* Assignment within a condition */
#endif

static char**
cmd_create(int dummy, ...)
{
  va_list ap;
  va_list ap_cp;
  const char* str;
  size_t i, n = 0;
  char** cmd = NULL;

  va_start(ap, dummy);
  VA_COPY(ap_cp, ap);
  while((str = va_arg(ap, const char*))) ++n;
  va_end(ap);

  CHK((cmd = sa_add(cmd, n)) != NULL);
  i = 0;
  while((str = va_arg(ap_cp, const char*))) {
    cmd[i] = NULL;
    CHK((cmd[i] = sa_add(cmd[i], strlen(str)+1)) != NULL);
    strcpy(cmd[i], str);
    ++i;
  }
  va_end(ap_cp);
  return cmd;
}

#ifdef COMPILER_CL
  #pragma warning(pop)
#endif

static void
cmd_delete(char** cmd)
{
  size_t i = 0;
  const size_t n = sa_size(cmd);
  FOR_EACH(i, 0, n) sa_release(cmd[i]);
  sa_release(cmd);
}

static FINLINE int
cmd_size(char** cmd)
{
  return (int)sa_size(cmd);
}

static void
test_rendering(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;
  double tmp[3];

  cmd = cmd_create(0, "test", "-r", "img=1280x720", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.rendering == 1);
  CHK(args.nexperiments == SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHK(d3_eq(args.camera.pos, SOLSTICE_ARGS_DEFAULT.camera.pos) == 1);
  CHK(d3_eq(args.camera.tgt, SOLSTICE_ARGS_DEFAULT.camera.tgt) == 1);
  CHK(d3_eq(args.camera.up, SOLSTICE_ARGS_DEFAULT.camera.up) == 1);
  CHK(args.camera.fov_x == SOLSTICE_ARGS_DEFAULT.camera.fov_x);
  CHK(args.img.width == 1280);
  CHK(args.img.height == 720);
  CHK(args.img.spp == SOLSTICE_ARGS_DEFAULT.img.spp);
  CHK(args.render_mode == SOLSTICE_ARGS_DEFAULT.render_mode);
  CHK(args.quiet == 0);
  CHK(args.output_filename == NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-q", "-r", "img=640x480:fov=70:pos=1,2,3", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.rendering == 1);
  CHK(args.nexperiments == SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHK(d3_eq(args.camera.pos, d3(tmp, 1, 2, 3)) == 1);
  CHK(d3_eq(args.camera.tgt, SOLSTICE_ARGS_DEFAULT.camera.tgt) == 1);
  CHK(d3_eq(args.camera.up, SOLSTICE_ARGS_DEFAULT.camera.up) == 1);
  CHK(args.img.width == 640);
  CHK(args.img.height == 480);
  CHK(args.img.spp == SOLSTICE_ARGS_DEFAULT.img.spp);
  CHK(args.render_mode == SOLSTICE_ARGS_DEFAULT.render_mode);
  CHK(args.quiet == 1);
  CHK(eq_eps(args.camera.fov_x, 70, 1.e-6) == 1);
  CHK(args.output_filename == NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,0,1:tgt=0,-10,0:rmode=draft", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nexperiments == SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHK(d3_eq(args.camera.pos, SOLSTICE_ARGS_DEFAULT.camera.pos) == 1);
  CHK(d3_eq(args.camera.tgt, d3(tmp, 0,-10, 0)) == 1);
  CHK(d3_eq(args.camera.up, d3(tmp, 0, 0, 1)) == 1);
  CHK(args.img.width == SOLSTICE_ARGS_DEFAULT.img.width);
  CHK(args.img.height == SOLSTICE_ARGS_DEFAULT.img.height);
  CHK(args.img.spp == SOLSTICE_ARGS_DEFAULT.img.spp);
  CHK(args.render_mode == SOLSTICE_ARGS_RENDER_DRAFT);
  CHK(args.rendering == 1);
  CHK(args.quiet == 0);
  CHK(args.output_filename == NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,0,1:rmode=pt:spp=4", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nexperiments == SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHK(d3_eq(args.camera.up, d3(tmp, 0, 0, 1)) == 1);
  CHK(args.img.width == SOLSTICE_ARGS_DEFAULT.img.width);
  CHK(args.img.height == SOLSTICE_ARGS_DEFAULT.img.height);
  CHK(args.rendering == 1);
  CHK(args.output_filename == NULL);
  CHK(args.img.spp == 4);
  CHK(args.render_mode == SOLSTICE_ARGS_RENDER_PATH_TRACING);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,10,0", "-o", "my_output", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(d3_eq(args.camera.up, d3(tmp, 0, 10, 0)) == 1);
  CHK(args.rendering == 1);
  CHK(args.quiet == 0);
  CHK(strcmp(args.output_filename, "my_output") ==  0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "spp=16", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.img.spp == 16);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "rmode=none", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "rmode", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);


  cmd = cmd_create(0, "test", "-r", "up=0,1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "tgt=0:1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "pos=0,10,1a", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "pos=0,10,1:::::", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(d3_eq(args.camera.pos, d3(tmp, 0, 10, 1)) == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "spp=0", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=32X32", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=32x32@12", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=0x64", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=64x0", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=32x32@12:up=0,0,1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "tgt=1,1,1:img=32x32:12", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "fov=123a", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "fov::::", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "::tgt", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_sun_dirs(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nsun_dirs == 1);
  CHK(args.sun_dirs[0].azimuth == 0);
  CHK(eq_eps(args.sun_dirs[0].elevation, 1, 1.e-6) == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:3.14,0.123:", RES_OK);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nsun_dirs == 2);
  CHK(eq_eps(args.sun_dirs[0].azimuth, 1.2, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[0].elevation, 3.4, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[1].azimuth, 3.14, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[1].elevation, 0.123, 1.e-6) == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:3.14,0.123:2.01,23.1", RES_OK);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nsun_dirs == 3);
  CHK(eq_eps(args.sun_dirs[0].azimuth, 1.2, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[0].elevation, 3.4, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[1].azimuth, 3.14, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[1].elevation, 0.123, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[2].azimuth, 2.01, 1.e-6) == 1);
  CHK(eq_eps(args.sun_dirs[2].elevation, 23.1, 1.e-6) == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4,5", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:5.2,A", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "-0.1,2", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "360,2", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,-1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,91", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  solstice_args_release(&args);
  cmd_delete(cmd);

}

static void
test_realisations_count(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nexperiments == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nexperiments == SOLSTICE_ARGS_DEFAULT.nexperiments);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "123", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nexperiments == 123);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "0", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "3.14", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_threads_count(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nthreads == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "123", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nthreads == 123);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "-1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nthreads == UINT_MAX);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.nthreads == SOLSTICE_ARGS_DEFAULT.nthreads);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "0", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "3.14", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

}

static void
test_output(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.output_filename == NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-o", "my_output", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(strcmp(args.output_filename, "my_output") == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-o", "hello_world", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(strcmp(args.output_filename, "hello_world") == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-o", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_quiet(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.quiet == SOLSTICE_ARGS_DEFAULT.quiet);
  CHK(args.quiet == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-q", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.quiet == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_verbose(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.verbose == SOLSTICE_ARGS_DEFAULT.verbose);
  CHK(args.verbose == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-v", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.verbose == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_receivers(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.receivers_filename == SOLSTICE_ARGS_DEFAULT.receivers_filename);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-R", "my_receivers", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(strcmp(args.receivers_filename, "my_receivers") == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-R", "foo bar", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(strcmp(args.receivers_filename, "foo bar") == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-R", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_input(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.input_filename == SOLSTICE_ARGS_DEFAULT.input_filename);
  CHK(args.input_filename == NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "my_input", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(strcmp(args.input_filename, "my_input") == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_dump(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_format == SOLSTICE_ARGS_DUMP_NONE);
  CHK(args.dump_split_mode == SOLSTICE_ARGS_DUMP_SPLIT_NONE);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_format == SOLSTICE_ARGS_DUMP_OBJ);
  CHK(args.dump_split_mode == SOLSTICE_ARGS_DUMP_SPLIT_NONE);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "split=geometry:format=obj", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_format == SOLSTICE_ARGS_DUMP_OBJ);
  CHK(args.dump_split_mode == SOLSTICE_ARGS_DUMP_SPLIT_GEOMETRY);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj:split=object", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_format == SOLSTICE_ARGS_DUMP_OBJ);
  CHK(args.dump_split_mode == SOLSTICE_ARGS_DUMP_SPLIT_OBJECT);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj::::split=none", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_format == SOLSTICE_ARGS_DUMP_OBJ);
  CHK(args.dump_split_mode == SOLSTICE_ARGS_DUMP_SPLIT_NONE);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "split=object", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=stl", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj:dummy", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj:split", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj", "-r", "up=0,0,1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj", "-p", "default", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_dump_paths(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_paths == 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_paths == 1);
  CHK(args.infinite_ray_length == SOLSTICE_ARGS_DEFAULT.infinite_ray_length);
  CHK(args.sun_ray_length == SOLSTICE_ARGS_DEFAULT.sun_ray_length);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen=3.14:srlen=1.23", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_paths == 1);
  CHK(eq_eps(args.infinite_ray_length, 3.14, 1.e-6) == 1);
  CHK(eq_eps(args.sun_ray_length, 1.23, 1.e-6) == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen=0", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_paths == 1);
  CHK(eq_eps(args.infinite_ray_length, 0, 1.e-6) == 1);
  CHK(args.sun_ray_length == SOLSTICE_ARGS_DEFAULT.sun_ray_length);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=-4", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_paths == 1);
  CHK(args.infinite_ray_length == SOLSTICE_ARGS_DEFAULT.infinite_ray_length);
  CHK(eq_eps(args.sun_ray_length, -4, 1.e-6) == 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=3.14:default", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_paths == 1);
  CHK(args.infinite_ray_length == SOLSTICE_ARGS_DEFAULT.infinite_ray_length);
  CHK(args.sun_ray_length == SOLSTICE_ARGS_DEFAULT.sun_ray_length);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default:srlen=1:irlen=2:", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_OK);
  CHK(args.dump_paths == 1);
  CHK(args.sun_ray_length == 1);
  CHK(args.infinite_ray_length == 2);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen=", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=abcd", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "=abcd", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default:srlen=1:irlen=2:",
    "-r", "up=0,0,1", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default:srlen=1:irlen=2:",
    "-g", "format=obj", NULL);
  CHK(solstice_args_init(&args, cmd_size(cmd), cmd) == RES_BAD_ARG);
  cmd_delete(cmd);
}

int
main(int argc, char** argv)
{
  (void)argc, (void)argv;
  test_rendering();
  test_sun_dirs();
  test_realisations_count();
  test_threads_count();
  test_output();
  test_quiet();
  test_verbose();
  test_receivers();
  test_input();
  test_dump();
  test_dump_paths();
  CHK(mem_allocated_size() == 0);
  return 0;
}

