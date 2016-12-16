/* Copyright (C) CNRS 2016
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

int
main(int argc, char** argv)
{
  struct solstice_args args = SOLSTICE_ARGS_NULL;
  char** cmd = NULL;
  double tmp[3];
  (void)argc, (void)argv;

  cmd = cmd_create(0, "test", "-r", "img=1280x720", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.img.width, 1280);
  CHECK(args.img.height, 720);
  CHECK(args.rendering, 1);
  CHECK(args.quiet, 0);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-q", "-r", "img=640x480:fov=70:pos=1,2,3", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(args.img.width, 640);
  CHECK(args.img.height, 480);
  CHECK(args.rendering, 1);
  CHECK(args.quiet, 1);
  CHECK(d3_eq(args.camera.pos, d3(tmp, 1, 2, 3)), 1);
  CHECK(eq_eps(args.camera.fov_x, MDEG2RAD(70), 1.e-6), 1);
  CHECK(args.output_filename, NULL);
  solstice_args_release(&args);
  cmd_delete(cmd);

  cmd = cmd_create(0, "test", "-r", "up=0,0,1:tgt=0,-10,0", NULL);
  CHECK(solstice_args_init(&args, cmd_size(cmd), cmd), RES_OK);
  CHECK(d3_eq(args.camera.tgt, d3(tmp, 0,-10, 0)), 1);
  CHECK(d3_eq(args.camera.up, d3(tmp, 0, 0, 1)), 1);
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

  CHECK(mem_allocated_size(), 0);
  return 0;
}

