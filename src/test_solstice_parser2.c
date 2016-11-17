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

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solstice_parser* parser;
  struct solstice_entity_iterator it, end;
  struct solstice_entity_id entity_id;
  const struct solstice_entity* entity, *entity1, *entity2;
  const struct solstice_geometry* geom;
  double tmp[3];

  FILE* stream;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solstice_parser_create(&allocator, &parser);

  stream = tmpfile();
  NCHECK(stream, NULL);

  fprintf(stream, "- geometry: &sphere\n");
  fprintf(stream, "    - sphere: { radius: 1  }\n");
  fprintf(stream, "      material: { matte: { reflectivity: 1 } }\n");
  fprintf(stream, "\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: lvl0\n");
  fprintf(stream, "    geometry: *sphere\n");
  fprintf(stream, "    transform: { translation: [1,2,3], rotation: [4,5,6]}\n");
  fprintf(stream, "    children:\n");
  fprintf(stream, "      - name: lvl1a\n");
  fprintf(stream, "        geometry: \n");
  fprintf(stream, "          - sphere: {radius: 2}\n");
  fprintf(stream, "            material: { matte: {reflectivity: 0.5}}\n");
  fprintf(stream, "      - name: lvl1b\n");
  fprintf(stream, "        geometry: *sphere\n");
  fprintf(stream, "        transform: { rotation: [3.14, 0, -1] }\n");
  fprintf(stream, "        children:\n");
  fprintf(stream, "          - name: lvl2\n");
  fprintf(stream, "            geometry: *sphere\n");
  rewind(stream);

  CHECK(solstice_parser_setup(parser, NULL, stream), RES_OK);
  CHECK(solstice_parser_load(parser), RES_OK);

  solstice_parser_entity_iterator_begin(parser, &it);
  solstice_parser_entity_iterator_end(parser, &end);
  CHECK(solstice_entity_iterator_eq(&it, &end), 0);

  entity_id = solstice_entity_iterator_get(&it);
  entity = solstice_parser_get_entity(parser, entity_id);

  solstice_entity_iterator_next(&it);
  CHECK(solstice_entity_iterator_eq(&it, &end), 1);

  CHECK(d3_eq(entity->translation, d3(tmp, 1, 2, 3)), 1);
  CHECK(d3_eq(entity->rotation, d3(tmp, 4, 5, 6)), 1);
  CHECK(strcmp("lvl0", str_cget(&entity->name)), 0);
  CHECK(solstice_entity_get_children_count(entity), 2);
  geom = solstice_parser_get_geometry(parser, entity->geometry);

  entity_id = solstice_entity_get_child(entity, 0);
  entity1 = solstice_parser_get_entity(parser, entity_id);
  CHECK(d3_eq(entity1->translation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity1->rotation, d3_splat(tmp, 0)), 1);
  CHECK(strcmp("lvl1a", str_cget(&entity1->name)), 0);
  CHECK(solstice_entity_get_children_count(entity1), 0);

  entity_id = solstice_entity_get_child(entity, 1);
  entity1 = solstice_parser_get_entity(parser, entity_id);
  CHECK(d3_eq(entity1->translation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity1->rotation, d3(tmp, 3.14, 0, -1)), 1);
  CHECK(strcmp("lvl1b", str_cget(&entity1->name)), 0);
  CHECK(solstice_entity_get_children_count(entity1), 1);

  entity_id = solstice_entity_get_child(entity1, 0);
  entity2 = solstice_parser_get_entity(parser, entity_id);
  CHECK(d3_eq(entity2->translation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity2->rotation, d3_splat(tmp, 0)), 1);
  CHECK(strcmp("lvl2", str_cget(&entity2->name)), 0);
  CHECK(solstice_entity_get_children_count(entity2), 0);

  CHECK(solstice_parser_load(parser), RES_BAD_OP);
  solstice_parser_ref_put(parser);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
