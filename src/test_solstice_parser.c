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

#include "solstice_parser.h"
#include "test_solstice_utils.h"

#include <string.h>

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solstice_parser* parser;
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
      printf("  -e check that the submitted FILEs has errors.\n");
      return 0;
    }
  }

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solstice_parser_create(&allocator, &parser);

  CHECK(solstice_parser_setup(parser, NULL, tmpfile()), RES_OK);
  CHECK(solstice_parser_setup(parser, "yop", tmpfile()), RES_OK);
  CHECK(solstice_parser_load(parser), RES_BAD_OP); /* Empty stream */

  FOR_EACH(i, ifile, argc) {
    FILE* file = fopen(argv[i], "rb");
    NCHECK(file, NULL);
    CHECK(solstice_parser_setup(parser, argv[i], file), RES_OK);
    for(;;) {
      const res_T res = solstice_parser_load(parser);
      if(res == RES_BAD_OP) break;
      CHECK(res, load_res);
    }
    fclose(file);
  }

  solstice_parser_ref_get(parser);
  solstice_parser_ref_put(parser);
  solstice_parser_ref_put(parser);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
