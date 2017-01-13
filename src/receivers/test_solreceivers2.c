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

#include <string.h>

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solreceivers* receivers;
  struct solreceiver receiver;
  FILE* stream;
  int seek;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solreceivers_create(&allocator, &receivers);

  stream = tmpfile();
  NCHECK(stream, NULL);
  fprintf(stream, "- { name: entity0 }\n");
  fprintf(stream, "- { name: \"entity1\" }\n");
  fprintf(stream, "- { name: entity2, side: FRONT }\n");
  fprintf(stream, "- { name: entity3, side: BACK }\n");
  fprintf(stream, "- name: entity4\n");
  fprintf(stream, "  side: FRONT_AND_BACK\n");
  rewind(stream);

  CHECK(solreceivers_setup_stream(receivers, NULL, stream), RES_OK);
  CHECK(solreceivers_load(receivers), RES_OK);
  CHECK(solreceivers_count(receivers), 5);

  solreceivers_get(receivers, 0, &receiver);
  CHECK(strcmp(receiver.name, "entity0"), 0);
  CHECK(receiver.side, SOLRECEIVER_FRONT_AND_BACK);

  solreceivers_get(receivers, 1, &receiver);
  CHECK(strcmp(receiver.name, "entity1"), 0);
  CHECK(receiver.side, SOLRECEIVER_FRONT_AND_BACK);

  solreceivers_get(receivers, 2, &receiver);
  CHECK(strcmp(receiver.name, "entity2"), 0);
  CHECK(receiver.side, SOLRECEIVER_FRONT);

  solreceivers_get(receivers, 3, &receiver);
  CHECK(strcmp(receiver.name, "entity3"), 0);
  CHECK(receiver.side, SOLRECEIVER_BACK);

  solreceivers_get(receivers, 4, &receiver);
  CHECK(strcmp(receiver.name, "entity4"), 0);
  CHECK(receiver.side, SOLRECEIVER_FRONT_AND_BACK);

  CHECK(solreceivers_load(receivers), RES_BAD_OP);

  seek = (int)ftell(stream);
  fprintf(stream, "---\n");
  fprintf(stream, "[{name: test 0, side: FRONT}, {name: test 1, side: BACK}]\n");
  fseek(stream, seek, SEEK_SET);

  CHECK(solreceivers_setup_stream(receivers, NULL, stream), RES_OK);
  CHECK(solreceivers_load(receivers), RES_OK);
  CHECK(solreceivers_count(receivers), 2);

  solreceivers_get(receivers, 0, &receiver);
  CHECK(strcmp(receiver.name, "test 0"), 0);
  CHECK(receiver.side, SOLRECEIVER_FRONT);

  solreceivers_get(receivers, 1, &receiver);
  CHECK(strcmp(receiver.name, "test 1"), 0);
  CHECK(receiver.side, SOLRECEIVER_BACK);

  fclose(stream);
  solreceivers_ref_put(receivers);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
