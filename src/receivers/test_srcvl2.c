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

#include "srcvl.h"
#include "test_solstice_utils.h"

#include <string.h>

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct srcvl* srcvl;
  struct srcvl_receiver receiver;
  FILE* stream;
  int seek;
  (void)argc, (void)argv;

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  srcvl_create(&allocator, &srcvl);

  stream = tmpfile();
  CHK(stream != NULL);
  fprintf(stream, "- { name: entity0 }\n");
  fprintf(stream, "- { name: \"entity1\" }\n");
  fprintf(stream, "- { name: entity2, side: FRONT }\n");
  fprintf(stream, "- { name: entity3, side: BACK }\n");
  fprintf(stream, "- name: entity4\n");
  fprintf(stream, "  side: FRONT_AND_BACK\n");
  fprintf(stream, "- { name: entity5, side: BACK, per_primitive: 1 }\n");
  fprintf(stream, "- { name: entity6, per_primitive: 0, side: FRONT }\n");
  rewind(stream);

  CHK(srcvl_setup_stream(srcvl, NULL, stream) == RES_OK);
  CHK(srcvl_load(srcvl) == RES_OK);
  CHK(srcvl_count(srcvl) == 7);

  srcvl_get(srcvl, 0, &receiver);
  CHK(strcmp(receiver.name, "entity0") == 0);
  CHK(receiver.side == SRCVL_FRONT_AND_BACK);
  CHK(receiver.per_primitive == 0);

  srcvl_get(srcvl, 1, &receiver);
  CHK(strcmp(receiver.name, "entity1") == 0);
  CHK(receiver.side == SRCVL_FRONT_AND_BACK);
  CHK(receiver.per_primitive == 0);

  srcvl_get(srcvl, 2, &receiver);
  CHK(strcmp(receiver.name, "entity2") == 0);
  CHK(receiver.side == SRCVL_FRONT);
  CHK(receiver.per_primitive == 0);

  srcvl_get(srcvl, 3, &receiver);
  CHK(strcmp(receiver.name, "entity3") == 0);
  CHK(receiver.side == SRCVL_BACK);
  CHK(receiver.per_primitive == 0);

  srcvl_get(srcvl, 4, &receiver);
  CHK(strcmp(receiver.name, "entity4") == 0);
  CHK(receiver.side == SRCVL_FRONT_AND_BACK);
  CHK(receiver.per_primitive == 0);

  srcvl_get(srcvl, 5, &receiver);
  CHK(strcmp(receiver.name, "entity5") == 0);
  CHK(receiver.side == SRCVL_BACK);
  CHK(receiver.per_primitive == 1);

  srcvl_get(srcvl, 6, &receiver);
  CHK(strcmp(receiver.name, "entity6") == 0);
  CHK(receiver.side == SRCVL_FRONT);
  CHK(receiver.per_primitive == 0);

  CHK(srcvl_load(srcvl) == RES_BAD_OP);

  seek = (int)ftell(stream);
  fprintf(stream, "---\n");
  fprintf(stream, "[{name: test 0, side: FRONT}, {name: test 1, side: BACK}]\n");
  fseek(stream, seek, SEEK_SET);

  CHK(srcvl_setup_stream(srcvl, NULL, stream) == RES_OK);
  CHK(srcvl_load(srcvl) == RES_OK);
  CHK(srcvl_count(srcvl) == 2);

  srcvl_get(srcvl, 0, &receiver);
  CHK(strcmp(receiver.name, "test 0") == 0);
  CHK(receiver.side == SRCVL_FRONT);

  srcvl_get(srcvl, 1, &receiver);
  CHK(strcmp(receiver.name, "test 1") == 0);
  CHK(receiver.side == SRCVL_BACK);

  fclose(stream);
  srcvl_ref_put(srcvl);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}
