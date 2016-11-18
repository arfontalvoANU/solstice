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
  struct solstice_object_id obj_id;
  struct solstice_geometry_id geom_id;
  const struct solstice_entity* entity, *entity1a, *entity1b, *entity2, *entity3;
  const struct solstice_geometry* geom;
  const struct solstice_object* obj;
  const struct solstice_shape* shape;
  const struct solstice_material_double_sided* mtl2;
  const struct solstice_material* mtl;
  const struct solstice_material_matte* matte;
  const struct solstice_material_mirror* mirror;
  const struct solstice_shape_sphere* sphere;
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
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: lvl 0\n");
  fprintf(stream, "    geometry: *sphere\n");
  fprintf(stream, "    transform: { translation: [1,2,3], rotation: [4,5,6]}\n");
  fprintf(stream, "    children:\n");
  fprintf(stream, "      - name: lvl1a\n");
  fprintf(stream, "        geometry: \n");
  fprintf(stream, "          - sphere: {radius: 2}\n");
  fprintf(stream, "            material:\n");
  fprintf(stream, "              mirror: { reflectivity: 0.9, roughness: 0.1 }\n");
  fprintf(stream, "      - name: lvl1b\n");
  fprintf(stream, "        geometry: *sphere\n");
  fprintf(stream, "        transform: { rotation: [3.14, 0, -1] }\n");
  fprintf(stream, "        children:\n");
  fprintf(stream, "          - name: lvl2\n");
  fprintf(stream, "            geometry: *sphere\n");
  fprintf(stream, "- sun:\n");
  fprintf(stream, "    dni: 1\n");
  fprintf(stream, "    spectrum: [ { wavelength: 1, data: 1} ]\n");
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
  CHECK(strcmp("lvl 0", str_cget(&entity->name)), 0);
  CHECK(solstice_entity_get_children_count(entity), 2);
  geom_id = entity->geometry;
  geom = solstice_parser_get_geometry(parser, entity->geometry);
  CHECK(solstice_geometry_get_objects_count(geom), 1);
  obj_id = solstice_geometry_get_object(geom, 0);
  obj = solstice_parser_get_object(parser, obj_id);
  CHECK(d3_eq(obj->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(obj->translation, d3_splat(tmp, 0)), 1);
  shape = solstice_parser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLSTICE_SHAPE_SPHERE);
  sphere = solstice_parser_get_shape_sphere(parser, shape->data.sphere);
  CHECK(sphere->radius, 1);
  mtl2 = solstice_parser_get_material_double_sided(parser, obj->mtl2);
  CHECK(mtl2->front.i, mtl2->back.i);
  mtl = solstice_parser_get_material(parser, mtl2->front);
  CHECK(mtl->type, SOLSTICE_MATERIAL_MATTE);
  matte = solstice_parser_get_material_matte(parser, mtl->data.matte);
  CHECK(matte->reflectivity, 1);

  entity_id = solstice_entity_get_child(entity, 0);
  entity1a = solstice_parser_get_entity(parser, entity_id);
  CHECK(d3_eq(entity1a->translation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity1a->rotation, d3_splat(tmp, 0)), 1);
  CHECK(strcmp("lvl1a", str_cget(&entity1a->name)), 0);
  CHECK(solstice_entity_get_children_count(entity1a), 0);
  NCHECK(entity1a->geometry.i, geom_id.i);
  geom = solstice_parser_get_geometry(parser, entity1a->geometry);
  CHECK(solstice_geometry_get_objects_count(geom), 1);
  obj_id = solstice_geometry_get_object(geom, 0);
  obj = solstice_parser_get_object(parser, obj_id);
  CHECK(d3_eq(obj->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(obj->translation, d3_splat(tmp, 0)), 1);
  shape = solstice_parser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLSTICE_SHAPE_SPHERE);
  sphere = solstice_parser_get_shape_sphere(parser, shape->data.sphere);
  CHECK(sphere->radius, 2);
  mtl2 = solstice_parser_get_material_double_sided(parser, obj->mtl2);
  CHECK(mtl2->front.i, mtl2->back.i);
  mtl = solstice_parser_get_material(parser, mtl2->front);
  CHECK(mtl->type, SOLSTICE_MATERIAL_MIRROR);
  mirror = solstice_parser_get_material_mirror(parser, mtl->data.mirror);
  CHECK(mirror->reflectivity, 0.9);
  CHECK(mirror->roughness, 0.1);

  entity_id = solstice_entity_get_child(entity, 1);
  entity1b = solstice_parser_get_entity(parser, entity_id);
  CHECK(d3_eq(entity1b->translation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity1b->rotation, d3(tmp, 3.14, 0, -1)), 1);
  CHECK(strcmp("lvl1b", str_cget(&entity1b->name)), 0);
  CHECK(solstice_entity_get_children_count(entity1b), 1);
  CHECK(entity1b->geometry.i, geom_id.i);

  entity_id = solstice_entity_get_child(entity1b, 0);
  entity2 = solstice_parser_get_entity(parser, entity_id);
  CHECK(d3_eq(entity2->translation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity2->rotation, d3_splat(tmp, 0)), 1);
  CHECK(strcmp("lvl2", str_cget(&entity2->name)), 0);
  CHECK(solstice_entity_get_children_count(entity2), 0);
  CHECK(entity2->geometry.i, geom_id.i);

  entity3 = solstice_parser_find_entity(parser, "lvl 0");
  CHECK(entity3, entity);
  entity3 = solstice_parser_find_entity(parser, "lvl1a");
  CHECK(entity3, NULL);
  entity3 = solstice_parser_find_entity(parser, "lvl 0.lvl1a");
  CHECK(entity3, entity1a);
  entity3 = solstice_parser_find_entity(parser, "lvl 0.lvl1b");
  CHECK(entity3, entity1b);
  entity3 = solstice_parser_find_entity(parser, "lvl 0.lvl1b.lvl2");
  CHECK(entity3, entity2);

  CHECK(solstice_parser_load(parser), RES_BAD_OP);
  solstice_parser_ref_put(parser);

  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}

