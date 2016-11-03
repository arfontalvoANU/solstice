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

#include "solstice_facility.h"
#include <rsys/rsys.h>

int
main(int argc, char** argv)
{
  res_T res;
  int err;
  int i;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s FILE [FILE ...]\n", argv[0]);
    err = 1;
    goto error;
  }

  FOR_EACH(i, 1, argc) {
    res = solstice_facility_load(argv[i]);
    if(res != RES_OK) {
      err = 1;
      goto error;
    }
  }

exit:
  return err;
error:
  goto exit;
}
