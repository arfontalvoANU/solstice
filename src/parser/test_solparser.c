/* Copyright (C) 2016-2018 CNRS
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

#include "solparser.h"
#include "test_solstice_utils.h"

#include <string.h>

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
  int ifile = 1;
  int i;
  res_T load_res = RES_OK;
  (void)argc, (void)argv;

  if(argc > 1) {
    if(!strcmp(argv[1], "-e")) {
      load_res = RES_BAD_ARG;
      ifile = 2;
    } else if(!strcmp(argv[1], "-h")) {
      printf("Usage: %s [OPTIONS] [FILE ... ]\n", argv[0]);
      printf(
"Check the parser API and that the submitted FILEs are valid. Use the `-e'\n"
"option to check that the FILEs are invalid.\n\n");
      printf("OPTIONS:\n");
      printf("  -h print this help and exit.\n");
      printf("  -e check that the submitted FILEs have errors.\n");
      return 0;
    }
  }

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  solparser_create(&allocator, &parser);

  CHK(solparser_setup(parser, NULL, tmpfile()) == RES_OK);
  CHK(solparser_setup(parser, "yop", tmpfile()) == RES_OK);
  CHK(solparser_load(parser) == RES_BAD_OP); /* Empty stream */

  FOR_EACH(i, ifile, argc) {
    FILE* file = fopen(argv[i], "rb");
    int count = 0;
    CHK(file != NULL);
    CHK(solparser_setup(parser, argv[i], file) == RES_OK);
    for(;;) {
      const res_T res = solparser_load(parser);
      if(count == 0 && load_res == RES_OK) {
        CHK(res == RES_OK);
      } else if(res == RES_BAD_OP) {
        break;
      }
      CHK(res == load_res);
      ++count;
    }
    fclose(file);
  }

  solparser_ref_get(parser);
  solparser_ref_put(parser);
  solparser_ref_put(parser);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}
