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

const struct solstice_geometry* geometry;

static const char* input[] = {
  "- sun: \n",
  "    dni: 1\n",
  "    spectrum: [{wavelength: 1, data: 1}]\n",
  "- material: &lambertian\n",
  "    mirror: { reflectivity: 0.2, roughness: 0.1 }\n",
  "- geometry: &cuboid\n",
  "    - cuboid: { size: [1, 2, 3] }\n",
  "      material: *lambertian\n",
  "- entity-template: &template\n",
  "    name: template0\n",
  "    geometry: *cuboid\n",
  "- entity:\n",
  "    name: entity0\n",
  "    transform: { translation: [1, 2, 3] }\n",
  "    children: [ *template ]\n",
  "- entity:\n",
  "    name: entity1\n",
  "    transform: { translation: [3, 4, 5] }\n",
  "    children: [ *template ]\n",
  NULL
};

static void
check_entity0
  (struct solstice_parser* parser, const struct solstice_entity* entity0)
{
  const struct solstice_entity* entity;
  struct solstice_entity_id entity_id;
  double tmp[3];

  NCHECK(parser, NULL);
  NCHECK(entity0, NULL);

  CHECK(strcmp(str_cget(&entity0->name), "entity0"), 0);
  CHECK(d3_eq(entity0->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity0->translation, d3(tmp, 1, 2, 3)), 1);
  CHECK(entity0->type, SOLSTICE_ENTITY_EMPTY);

  CHECK(solstice_entity_get_children_count(entity0), 1);
  CHECK(solstice_entity_get_anchors_count(entity0), 0);

  entity_id = solstice_entity_get_child(entity0, 0);
  entity = solstice_parser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity->name), "template0"), 0);
  CHECK(d3_eq(entity->translation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity->rotation, d3_splat(tmp, 0)), 1);

  CHECK(entity->type, SOLSTICE_ENTITY_GEOMETRY);
  CHECK(solstice_parser_get_geometry(parser, entity->data.geometry), geometry);
  CHECK(solstice_parser_find_entity(parser, "entity0.template0"), entity);
}

static void
check_entity1
  (struct solstice_parser* parser, const struct solstice_entity* entity1)
{
  const struct solstice_entity* entity;
  struct solstice_entity_id entity_id;
  double tmp[3];

  NCHECK(parser, NULL);
  NCHECK(entity1, NULL);

  CHECK(strcmp(str_cget(&entity1->name), "entity1"), 0);
  CHECK(d3_eq(entity1->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity1->translation, d3(tmp, 3, 4, 5)), 1);
  CHECK(entity1->type, SOLSTICE_ENTITY_EMPTY);

  CHECK(solstice_entity_get_children_count(entity1), 1);
  CHECK(solstice_entity_get_anchors_count(entity1), 0);

  entity_id = solstice_entity_get_child(entity1, 0);
  entity = solstice_parser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity->name), "template0"), 0);
  CHECK(d3_eq(entity->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity->translation, d3_splat(tmp, 0)), 1);

  CHECK(entity->type, SOLSTICE_ENTITY_GEOMETRY);
  CHECK(solstice_parser_get_geometry(parser, entity->data.geometry), geometry);
  CHECK(solstice_parser_find_entity(parser, "entity1.template0"), entity);
}

int
main(int argc, char** argv)
{
  struct solstice_entity_iterator it, it_end;
  struct solstice_geometry_iterator it_geom, it_end_geom;
  struct mem_allocator allocator;
  struct solstice_parser* parser;
  const struct solstice_material* mtl;
  const struct solstice_material_double_sided* mtl2;
  const struct solstice_material_mirror* mirror;
  const struct solstice_object* obj;
  const struct solstice_shape* shape;
  const struct solstice_shape_cuboid* cuboid;
  struct solstice_geometry_id geom_id;
  struct solstice_object_id obj_id;
  double tmp[3];
  FILE* stream;
  size_t i;
  int entity0 = 0;
  int entity1 = 0;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solstice_parser_create(&allocator, &parser);

  stream = tmpfile();
  NCHECK(stream, NULL);
  i = 0;
  while(input[i]) {
    const size_t len = strlen(input[i]);
    CHECK(fwrite(input[i], 1, len, stream), len);
    ++i;
  }
  rewind(stream);

  CHECK(solstice_parser_setup(parser, NULL, stream), RES_OK);
  CHECK(solstice_parser_load(parser), RES_OK);

  solstice_parser_geometry_iterator_begin(parser, &it_geom);
  solstice_parser_geometry_iterator_end(parser, &it_end_geom);
  CHECK(solstice_entity_iterator_eq(&it, &it_end), 0);
  geom_id = solstice_geometry_iterator_get(&it_geom);
  geometry = solstice_parser_get_geometry(parser, geom_id);
  solstice_geometry_iterator_next(&it_geom);
  CHECK(solstice_geometry_iterator_eq(&it_geom, &it_end_geom), 1);

  CHECK(solstice_geometry_get_objects_count(geometry), 1);
  obj_id = solstice_geometry_get_object(geometry, 0);
  obj = solstice_parser_get_object(parser, obj_id);
  CHECK(d3_eq(obj->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(obj->translation, d3_splat(tmp, 0)), 1);

  mtl2 = solstice_parser_get_material_double_sided(parser, obj->mtl2);
  CHECK(mtl2->front.i, mtl2->back.i);
  mtl = solstice_parser_get_material(parser, mtl2->front);
  CHECK(mtl->type, SOLSTICE_MATERIAL_MIRROR);
  mirror = solstice_parser_get_material_mirror(parser, mtl->data.mirror);
  CHECK(mirror->reflectivity, 0.2);
  CHECK(mirror->roughness, 0.1);

  shape = solstice_parser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLSTICE_SHAPE_CUBOID);
  cuboid = solstice_parser_get_shape_cuboid(parser, shape->data.cuboid);
  CHECK(cuboid->size[0], 1);
  CHECK(cuboid->size[1], 2);
  CHECK(cuboid->size[2], 3);

  solstice_parser_entity_iterator_begin(parser, &it);
  solstice_parser_entity_iterator_end(parser, &it_end);
  CHECK(solstice_entity_iterator_eq(&it, &it_end), 0);

  while(!solstice_entity_iterator_eq(&it, &it_end)) {
    struct solstice_entity_id entity_id;
    const struct solstice_entity* entity;

    entity_id = solstice_entity_iterator_get(&it);
    entity = solstice_parser_get_entity(parser, entity_id);

    if(!strcmp(str_cget(&entity->name), "entity0")) {
      CHECK(entity0, 0);
      entity0 = 1;
      check_entity0(parser, entity);
    } else if(!strcmp(str_cget(&entity->name), "entity1")) {
      CHECK(entity1, 0);
      entity1 = 1;
      check_entity1(parser, entity);
    } else {
      FATAL("Unexpected entity name.\n");
    }

    solstice_entity_iterator_next(&it);
  }
  CHECK(entity0, 1);
  CHECK(entity1, 1);

  CHECK(solstice_parser_load(parser), RES_BAD_OP);
  solstice_parser_ref_put(parser);
  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}

