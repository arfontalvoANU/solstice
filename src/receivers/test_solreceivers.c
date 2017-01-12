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

#include "solreceivers.h"
#include "test_solstice_utils.h"

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solreceivers* receivers;
  int ifile = 1;
  int i;
  res_T load_res = RES_OK;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solreceivers_create(&allocator, &receivers);

  CHECK(solreceivers_setup_stream(receivers, NULL, tmpfile()), RES_OK);
  CHECK(solreceivers_setup_stream(receivers, "yop", tmpfile()), RES_OK);
  CHECK(solreceivers_load(receivers), RES_BAD_OP); /* Empty stream */

  FOR_EACH(i, ifile, argc) {
    FILE* file = fopen(argv[i], "rb");
    int count = 0;
    NCHECK(file, NULL);
    CHECK(solreceivers_setup_stream(receivers, argv[i], file), RES_OK);
    for(;;) {
      const res_T res = solreceivers_load(receivers);
      if(count == 0 && load_res == RES_OK) {
        CHECK(res, RES_OK);
      } else if(res == RES_BAD_OP) {
        break;
      }
      CHECK(res, load_res);
      ++count;
    }
    fclose(file);
  }
  solreceivers_ref_get(receivers);
  solreceivers_ref_put(receivers);
  solreceivers_ref_put(receivers);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}

