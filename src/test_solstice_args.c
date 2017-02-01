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
  CHECK(args.nrealisations, SOLSTICE_ARGS_DEFAULT.nrealisations);
  CHECK(d3_eq(args.camera.pos, SOLSTICE_ARGS_DEFAULT.camera.pos), 1);
  CHECK(d3_eq(args.camera.tgt, SOLSTICE_ARGS_DEFAULT.camera.tgt), 1);
  CHECK(d3_eq(args.camera.up, SOLSTICE_ARGS_DEFAULT.camera.up), 1);
  CHECK(args.camera.fov_x, SOLSTICE_ARGS_DEFAULT.camera.fov_x);
  CHECK(args.img.width, 1280);
  CHECK(args.img.height, 720);
  CHECK(args.quiet, 0);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-q", "-r", "img=640x480:fov=70:pos=1,2,3", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.rendering, 1);
  CHECK(args.nrealisations, SOLSTICE_ARGS_DEFAULT.nrealisations);
  CHECK(d3_eq(args.camera.pos, d3(tmp, 1, 2, 3)), 1);
  CHECK(d3_eq(args.camera.tgt, SOLSTICE_ARGS_DEFAULT.camera.tgt), 1);
  CHECK(d3_eq(args.camera.up, SOLSTICE_ARGS_DEFAULT.camera.up), 1);
  CHECK(args.img.width, 640);
  CHECK(args.img.height, 480);
  CHECK(args.quiet, 1);
  CHECK(eq_eps(args.camera.fov_x, 70, 1.e-6), 1);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,0,1:tgt=0,-10,0", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nrealisations, SOLSTICE_ARGS_DEFAULT.nrealisations);
  CHECK(d3_eq(args.camera.pos, SOLSTICE_ARGS_DEFAULT.camera.pos), 1);
  CHECK(d3_eq(args.camera.tgt, d3(tmp, 0,-10, 0)), 1);
  CHECK(d3_eq(args.camera.up, d3(tmp, 0, 0, 1)), 1);
  CHECK(args.img.width, SOLSTICE_ARGS_DEFAULT.img.width);
  CHECK(args.img.height, SOLSTICE_ARGS_DEFAULT.img.height);
  CHECK(args.rendering, 1);
  CHECK(args.quiet, 0);
  CHECK(args.output_filename, NULL);
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

  cmd = cmd_create(0, "test", "-r", "img=32X32", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "img=32x32@12", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "tgt=1,1,1:img=32x32:12", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_BAD_ARG);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "fov=123a", NULL);
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
  CHECK(args.nrealisations, 1);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nrealisations, SOLSTICE_ARGS_DEFAULT.nrealisations);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-n", "123", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.nrealisations, 123);
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
test_output_hits(void)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;

  cmd = cmd_create(0, "test", "-D", "0,90", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.output_hits, SOLSTICE_ARGS_DEFAULT.output_hits);
  CHECK(args.output_hits, 0);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-D", "0,90", "-H", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.output_hits, 1);
  solstice_args_release(&args);
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

int
main(int argc, char** argv)
{
  (void)argc, (void)argv;
  test_rendering();
  test_sun_dirs();
  test_realisations_count();
  test_threads_count();
  test_output_hits();
  test_output();
  test_quiet();
  test_receivers();
  test_input();
  CHECK(mem_allocated_size(), 0);
  return 0;
}

