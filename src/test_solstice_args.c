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

  NCHECK(cmd = sa_add(cmd, n), NULL);
  i = 0;
  while((str = va_arg(ap_cp, const char*))) {
    cmd[i] = NULL;
    NCHECK(cmd[i] = sa_add(cmd[i], strlen(str)+1), NULL);
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
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.rendering, 1);
  CHECK(args.nexperiments, SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHECK(d3_eq(args.camera.pos, SOLSTICE_ARGS_DEFAULT.camera.pos), 1);
  CHECK(d3_eq(args.camera.tgt, SOLSTICE_ARGS_DEFAULT.camera.tgt), 1);
  CHECK(d3_eq(args.camera.up, SOLSTICE_ARGS_DEFAULT.camera.up), 1);
  CHECK(args.camera.fov_x, SOLSTICE_ARGS_DEFAULT.camera.fov_x);
  CHECK(args.img.width, 1280);
  CHECK(args.img.height, 720);
  CHECK(args.img.spp, SOLSTICE_ARGS_DEFAULT.img.spp);
  CHECK(args.render_mode, SOLSTICE_ARGS_DEFAULT.render_mode);
  CHECK(args.quiet, 0);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-q", "-r", "img=640x480:fov=70:pos=1,2,3", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.rendering, 1);
  CHECK(args.nexperiments, SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHECK(d3_eq(args.camera.pos, d3(tmp, 1, 2, 3)), 1);
  CHECK(d3_eq(args.camera.tgt, SOLSTICE_ARGS_DEFAULT.camera.tgt), 1);
  CHECK(d3_eq(args.camera.up, SOLSTICE_ARGS_DEFAULT.camera.up), 1);
  CHECK(args.img.width, 640);
  CHECK(args.img.height, 480);
  CHECK(args.img.spp, SOLSTICE_ARGS_DEFAULT.img.spp);
  CHECK(args.render_mode, SOLSTICE_ARGS_DEFAULT.render_mode);
  CHECK(args.quiet, 1);
  CHECK(eq_eps(args.camera.fov_x, 70, 1.e-6), 1);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,0,1:tgt=0,-10,0:rmode=draft", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nexperiments, SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHECK(d3_eq(args.camera.pos, SOLSTICE_ARGS_DEFAULT.camera.pos), 1);
  CHECK(d3_eq(args.camera.tgt, d3(tmp, 0,-10, 0)), 1);
  CHECK(d3_eq(args.camera.up, d3(tmp, 0, 0, 1)), 1);
  CHECK(args.img.width, SOLSTICE_ARGS_DEFAULT.img.width);
  CHECK(args.img.height, SOLSTICE_ARGS_DEFAULT.img.height);
  CHECK(args.img.spp, SOLSTICE_ARGS_DEFAULT.img.spp);
  CHECK(args.render_mode, SOLSTICE_ARGS_RENDER_DRAFT);
  CHECK(args.rendering, 1);
  CHECK(args.quiet, 0);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,0,1:rmode=pt:spp=4", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nexperiments, SOLSTICE_ARGS_DEFAULT.nexperiments);
  CHECK(d3_eq(args.camera.up, d3(tmp, 0, 0, 1)), 1);
  CHECK(args.img.width, SOLSTICE_ARGS_DEFAULT.img.width);
  CHECK(args.img.height, SOLSTICE_ARGS_DEFAULT.img.height);
  CHECK(args.rendering, 1);
  CHECK(args.output_filename, NULL);
  CHECK(args.img.spp, 4);
  CHECK(args.render_mode, SOLSTICE_ARGS_RENDER_PATH_TRACING);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,10,0", "-o", "my_output", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(d3_eq(args.camera.up, d3(tmp, 0, 10, 0)), 1);
  CHECK(args.rendering, 1);
  CHECK(args.quiet, 0);
  CHECK(strcmp(args.output_filename, "my_output"),  0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "spp=16", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.img.spp, 16);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "rmode=none", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "rmode", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);


  cmd = cmd_create(0, "test", "-r", "up=0,1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "tgt=0:1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "pos=0,10,1a", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "pos=0,10,1:::::", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(d3_eq(args.camera.pos, d3(tmp, 0, 10, 1)), 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "spp=0", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=32X32", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=32x32@12", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=0x64", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=64x0", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=32x32@12:up=0,0,1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "tgt=1,1,1:img=32x32:12", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "fov=123a", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "fov::::", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "::tgt", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_sun_dirs(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nsun_dirs, 1);
  CHECK(args.sun_dirs[0].azimuth, 0);
  CHECK(eq_eps(args.sun_dirs[0].elevation, 1, 1.e-6), 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:3.14,0.123:", RES_OK);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nsun_dirs, 2);
  CHECK(eq_eps(args.sun_dirs[0].azimuth, 1.2, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[0].elevation, 3.4, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[1].azimuth, 3.14, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[1].elevation, 0.123, 1.e-6), 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:3.14,0.123:2.01,23.1", RES_OK);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nsun_dirs, 3);
  CHECK(eq_eps(args.sun_dirs[0].azimuth, 1.2, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[0].elevation, 3.4, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[1].azimuth, 3.14, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[1].elevation, 0.123, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[2].azimuth, 2.01, 1.e-6), 1);
  CHECK(eq_eps(args.sun_dirs[2].elevation, 23.1, 1.e-6), 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4,5", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "1.2,3.4:5.2,A", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "-0.1,2", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "360,2", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,-1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,91", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  solstice_args_release(&args);
  cmd_delete(cmd);

}

static void
test_realisations_count(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nexperiments, 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nexperiments, SOLSTICE_ARGS_DEFAULT.nexperiments);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "123", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nexperiments, 123);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "0", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "3.14", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_threads_count(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nthreads, 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "123", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nthreads, 123);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "-1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nthreads, UINT_MAX);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nthreads, SOLSTICE_ARGS_DEFAULT.nthreads);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "0", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", "3.14", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-t", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

}

static void
test_output(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-o", "my_output", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(strcmp(args.output_filename, "my_output"), 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-o", "hello_world", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(strcmp(args.output_filename, "hello_world"), 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-o", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_quiet(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.quiet, SOLSTICE_ARGS_DEFAULT.quiet);
  CHECK(args.quiet, 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-q", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.quiet, 1);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_verbose(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.verbose, SOLSTICE_ARGS_DEFAULT.verbose);
  CHECK(args.verbose, 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-v", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.verbose, 1);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_receivers(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.receivers_filename, SOLSTICE_ARGS_DEFAULT.receivers_filename);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-R", "my_receivers", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(strcmp(args.receivers_filename, "my_receivers"), 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-R", "foo bar", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(strcmp(args.receivers_filename, "foo bar"), 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-R", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_input(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.input_filename, SOLSTICE_ARGS_DEFAULT.input_filename);
  CHECK(args.input_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "my_input", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(strcmp(args.input_filename, "my_input"), 0);
  solstice_args_release(&args);
  cmd_delete(cmd);
}

static void
test_dump(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_format, SOLSTICE_ARGS_DUMP_NONE);
  CHECK(args.dump_split_mode, SOLSTICE_ARGS_DUMP_SPLIT_NONE);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_format, SOLSTICE_ARGS_DUMP_OBJ);
  CHECK(args.dump_split_mode, SOLSTICE_ARGS_DUMP_SPLIT_NONE);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "split=geometry:format=obj", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_format, SOLSTICE_ARGS_DUMP_OBJ);
  CHECK(args.dump_split_mode, SOLSTICE_ARGS_DUMP_SPLIT_GEOMETRY);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj:split=object", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_format, SOLSTICE_ARGS_DUMP_OBJ);
  CHECK(args.dump_split_mode, SOLSTICE_ARGS_DUMP_SPLIT_OBJECT);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj::::split=none", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_format, SOLSTICE_ARGS_DUMP_OBJ);
  CHECK(args.dump_split_mode, SOLSTICE_ARGS_DUMP_SPLIT_NONE);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "split=object", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=stl", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj:dummy", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj:split", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj", "-r", "up=0,0,1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-g", "format=obj", "-p", "default", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);
}

static void
test_dump_paths(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_paths, 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_paths, 1);
  CHECK(args.infinite_ray_length, SOLSTICE_ARGS_DEFAULT.infinite_ray_length);
  CHECK(args.sun_ray_length, SOLSTICE_ARGS_DEFAULT.sun_ray_length);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen=3.14:srlen=1.23", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_paths, 1);
  CHECK(eq_eps(args.infinite_ray_length, 3.14, 1.e-6), 1);
  CHECK(eq_eps(args.sun_ray_length, 1.23, 1.e-6), 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen=0", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_paths, 1);
  CHECK(eq_eps(args.infinite_ray_length, 0, 1.e-6), 1);
  CHECK(args.sun_ray_length, SOLSTICE_ARGS_DEFAULT.sun_ray_length);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=-4", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_paths, 1);
  CHECK(args.infinite_ray_length, SOLSTICE_ARGS_DEFAULT.infinite_ray_length);
  CHECK(eq_eps(args.sun_ray_length, -4, 1.e-6), 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=3.14:default", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_paths, 1);
  CHECK(args.infinite_ray_length, SOLSTICE_ARGS_DEFAULT.infinite_ray_length);
  CHECK(args.sun_ray_length, SOLSTICE_ARGS_DEFAULT.sun_ray_length);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default:srlen=1:irlen=2:", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.dump_paths, 1);
  CHECK(args.sun_ray_length, 1);
  CHECK(args.infinite_ray_length, 2);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen=", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "srlen=abcd", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "irlen", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "=abcd", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default:srlen=1:irlen=2:",
    "-r", "up=0,0,1", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-p", "default:srlen=1:irlen=2:",
    "-g", "format=obj", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
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
  CHECK(mem_allocated_size(), 0);
  return 0;
}

