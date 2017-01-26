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

#include "solparser.h"
#include "solparser_sun.h"
#include "test_solstice_utils.h"

static const char* input[] = {
  "- sun:\n",
  "    dni: 1\n",
  "    spectrum: [{wavelength: 1, data: 1}]\n",
  "- geometry: &cuboid\n",
  "    - cuboid: { size: [1, 2, 3] }\n",
  "      material: { matte: { reflectivity: 1 } }\n",
  "- template: &template\n",
  "    name: lvl1\n",
  "    primary: 1\n",
  "    geometry: *cuboid\n",
  "    anchors:\n",
  "      - name: anchor0\n",
  "        position: [1, 2, 3]\n",
  "    children:\n",
  "      - name: lvl2\n",
  "        pivot:\n",
  "          point: [1, 2, 3]\n",
  "          normal: [4, 5, 6]\n",
  "          target: { anchor: self.lvl1.anchor0 }\n",
  "- entity:\n",
  "    name: entity0\n",
  "    children: [ *template ]\n",
  "- entity:\n",
  "    name: entity1\n",
  "    children: [ *template ]\n",
  NULL
};

static void
check_entity(struct solparser* parser, const struct solparser_entity* entity)
{
  const struct solparser_anchor* anchor;
  const struct solparser_pivot* pivot;
  struct solparser_entity_id entity_id;
  double tmp[3];

  CHECK(entity->type, SOLPARSER_ENTITY_EMPTY);

  CHECK(solparser_entity_get_children_count(entity), 1);
  entity_id = solparser_entity_get_child(entity, 0);
  entity = solparser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity->name), "lvl1"), 0);
  CHECK(entity->type, SOLPARSER_ENTITY_GEOMETRY);

  CHECK(solparser_entity_get_children_count(entity), 1);
  entity_id = solparser_entity_get_child(entity, 0);
  entity = solparser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity->name), "lvl2"), 0);
  CHECK(entity->type, SOLPARSER_ENTITY_PIVOT);

  pivot = solparser_get_pivot(parser, entity->data.pivot);
  CHECK(d3_eq(pivot->point, d3(tmp, 1, 2, 3)), 1);
  CHECK(d3_eq(pivot->normal, d3(tmp, 4, 5, 6)), 1);
  CHECK(pivot->target_type, SOLPARSER_TARGET_ANCHOR);

  anchor = solparser_get_anchor(parser, pivot->target.anchor);
  CHECK(strcmp(str_cget(&anchor->name), "anchor0"), 0);
  CHECK(d3_eq(anchor->position, d3(tmp, 1, 2, 3)), 1);
}

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
  const struct solparser_entity* entity;
  struct solparser_entity_id entity_id;
  struct solparser_entity_iterator it, it_end;
  size_t i;
  int entity0 = 0;
  int entity1 = 0;
  FILE* stream;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solparser_create(&allocator, &parser);

  stream = tmpfile();
  NCHECK(stream, NULL);
  i = 0;
  while(input[i]) {
    const size_t len = strlen(input[i]);
    CHECK(fwrite(input[i], 1, len, stream), len);
    ++i;
  }
  rewind(stream);

  CHECK(solparser_setup(parser, NULL, stream), RES_OK);
  CHECK(solparser_load(parser), RES_OK);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &it_end);
  CHECK(solparser_entity_iterator_eq(&it, &it_end), 0);

  while(!solparser_entity_iterator_eq(&it, &it_end)) {
    entity_id = solparser_entity_iterator_get(&it);
    entity = solparser_get_entity(parser, entity_id);
    if(!strcmp(str_cget(&entity->name), "entity0")) {
      CHECK(entity0, 0);
      entity0 = 1;
      check_entity(parser, entity);
    } else if(!strcmp(str_cget(&entity->name), "entity1")) {
      CHECK(entity1, 0);
      entity1 = 1;
      check_entity(parser, entity);
    } else {
      FATAL("Unexpected entity name.\n");
    }
    solparser_entity_iterator_next(&it);
  }
  CHECK(entity0, 1);
  CHECK(entity1, 1);

  solparser_ref_put(parser);
  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}

