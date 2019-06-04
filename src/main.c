/* Copyright (C) 2016-2018 CNRS, 2018-2019 |Meso|Star>
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
#include "solstice_args.h"

#include <rsys/rsys.h>

int
main(int argc, char** argv)
{
  struct solstice_args args;
  struct solstice solstice;
  size_t memsz = 0;
  res_T res;
  int solstice_is_init = 0;
  int err = 0;

  res = solstice_args_init(&args, argc, argv);
  if(res != RES_OK) goto error;
  if(args.quit) goto exit;

  res = solstice_init(NULL, &args, &solstice);
  if(res != RES_OK) goto error;
  solstice_is_init = 1;

  res = solstice_run(&solstice);
  if(res != RES_OK) goto error;

exit:
  if(solstice_is_init) solstice_release(&solstice);
  solstice_args_release(&args);
  if((memsz = mem_allocated_size()) != 0) {
    fprintf(stderr, "Memory leaks: %lu Bytes\n", (unsigned long)memsz);
  }
  return err;
error:
  err = -1;
  goto exit;
}

