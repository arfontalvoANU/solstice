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

#include "parser/solparser.h"
#include <rsys/rsys.h>

int
main(int argc, char** argv)
{
  FILE* file = NULL;
  struct solparser* parser = NULL;
  res_T res;
  int err = 0;
  int i;

  if(argc < 2) {
    fprintf(stderr, "Usage: %s FILE [FILE ...]\n", argv[0]);
    err = 1;
    goto error;
  }

  res = solparser_create(NULL, &parser);
  if(res != RES_OK) goto error;

  FOR_EACH(i, 1, argc) {
    file = fopen(argv[i], "rb");
    if(!file) {
      fprintf(stderr, "Could not open the file `%s'.\n", argv[i]);
      goto error;
    }

    res = solparser_setup(parser, argv[i], file);
    if(res != RES_OK) break;

    do {
      res = solparser_load(parser);
    } while(res != RES_BAD_OP);

    fclose(file);
    file = NULL;
  }

exit:
  if(parser) solparser_ref_put(parser);
  if(file) fclose(file);
  return err;
error:
  err = -1;
  goto exit;
}

