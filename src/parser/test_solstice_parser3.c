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
  struct solstice_entity_iterator it, it_end;
  struct solstice_anchor_id anchor_id;
  struct solstice_entity_id entity_id;
  struct solstice_object_id obj_id;
  const struct solstice_anchor* anchor0, *anchor1, *anchor2, *anchor3, *anchor4;
  const struct solstice_entity* entity, *entity1;
  const struct solstice_geometry* geom;
  const struct solstice_material_matte* matte;
  const struct solstice_material* mtl;
  const struct solstice_material_double_sided* mtl2;
  const struct solstice_object* obj;
  const struct solstice_shape* shape;
  const struct solstice_shape_cylinder* cylinder;
  double tmp[3];
  FILE* stream;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solstice_parser_create(&allocator, &parser);

  stream = tmpfile();
  NCHECK(stream, NULL);

  fprintf(stream, "- material: &lambertian\n");
  fprintf(stream, "    matte: { reflectivity: 0.5 }\n");
  fprintf(stream, "- geometry: &cylinder\n");
  fprintf(stream, "    - cylinder: { radius: 1, height: 10, slices: 128 }\n");
  fprintf(stream, "      material: *lambertian\n");
  fprintf(stream, "- sun: \n");
  fprintf(stream, "    dni: 1\n");
  fprintf(stream, "    spectrum: [{wavelength: 1, data: 1}]\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: entity0\n");
  fprintf(stream, "    geometry: *cylinder\n");
  fprintf(stream, "    anchors:\n");
  fprintf(stream, "      - name: anchor0\n");
  fprintf(stream, "        position: [1, 2, 3]\n");
  fprintf(stream, "      - name: anchor1\n");
  fprintf(stream, "        position: [4, 5, 6]\n");
  fprintf(stream, "    children:\n");
  fprintf(stream, "      - name: entity0a\n");
  fprintf(stream, "        geometry: *cylinder\n");
  fprintf(stream, "      - name: entity0b\n");
  fprintf(stream, "        geometry: *cylinder\n");
  fprintf(stream, "        anchors:\n\n");
  fprintf(stream, "          - name: anchor0\n");
  fprintf(stream, "            position: [4, 5, 6]\n");
  fprintf(stream, "          - name: entity0b\n");
  fprintf(stream, "            position: [7, 8, 9]\n");

  rewind(stream);

  CHECK(solstice_parser_setup(parser, NULL, stream), RES_OK);
  CHECK(solstice_parser_load(parser), RES_OK);

  solstice_parser_entity_iterator_begin(parser, &it);
  solstice_parser_entity_iterator_end(parser, &it_end);
  CHECK(solstice_entity_iterator_eq(&it, &it_end), 0);

  entity_id = solstice_entity_iterator_get(&it);
  entity = solstice_parser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity->name), "entity0"), 0);
  CHECK(d3_eq(entity->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity->translation, d3_splat(tmp, 0)), 1);
  CHECK(entity->type, SOLSTICE_ENTITY_GEOMETRY);

  geom = solstice_parser_get_geometry(parser, entity->data.geometry);
  CHECK(solstice_geometry_get_objects_count(geom), 1);

  obj_id = solstice_geometry_get_object(geom, 0);
  obj = solstice_parser_get_object(parser, obj_id);
  CHECK(d3_eq(obj->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(obj->translation, d3_splat(tmp, 0)), 1);

  shape = solstice_parser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLSTICE_SHAPE_CYLINDER);

  cylinder = solstice_parser_get_shape_cylinder(parser, shape->data.cylinder);
  CHECK(cylinder->height, 10);
  CHECK(cylinder->radius, 1);
  CHECK(cylinder->nslices, 128);

  mtl2 = solstice_parser_get_material_double_sided(parser, obj->mtl2);
  CHECK(mtl2->front.i, mtl2->back.i);

  mtl = solstice_parser_get_material(parser, mtl2->front);
  CHECK(mtl->type, SOLSTICE_MATERIAL_MATTE);

  matte = solstice_parser_get_material_matte(parser, mtl->data.matte);
  CHECK(matte->reflectivity, 0.5);

  CHECK(solstice_entity_get_children_count(entity), 2);
  CHECK(solstice_entity_get_anchors_count(entity), 2);

  anchor_id = solstice_entity_get_anchor(entity, 0);
  anchor0 = solstice_parser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&anchor0->name), "anchor0"), 0);
  CHECK(d3_eq(anchor0->position, d3(tmp, 1, 2, 3)), 1);

  anchor_id = solstice_entity_get_anchor(entity, 1);
  anchor1 = solstice_parser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&anchor1->name), "anchor1"), 0);
  CHECK(d3_eq(anchor1->position, d3(tmp, 4, 5, 6)), 1);

  entity_id = solstice_entity_get_child(entity, 0);
  entity1 = solstice_parser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity1->name), "entity0a"), 0);
  CHECK(entity1->type, SOLSTICE_ENTITY_GEOMETRY);
  CHECK(entity->data.geometry.i, entity1->data.geometry.i);
  CHECK(solstice_entity_get_anchors_count(entity1), 0);
  CHECK(solstice_entity_get_children_count(entity1), 0);

  entity_id = solstice_entity_get_child(entity, 1);
  entity1 = solstice_parser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity1->name), "entity0b"), 0);
  CHECK(entity->type, SOLSTICE_ENTITY_GEOMETRY);
  CHECK(entity->data.geometry.i, entity1->data.geometry.i);
  CHECK(solstice_entity_get_anchors_count(entity1), 2);
  CHECK(solstice_entity_get_children_count(entity1), 0);

  anchor_id = solstice_entity_get_anchor(entity1, 0);
  anchor2 = solstice_parser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&anchor2->name), "anchor0"), 0);
  CHECK(d3_eq(anchor2->position, d3(tmp, 4, 5, 6)), 1);

  anchor_id = solstice_entity_get_anchor(entity1, 1);
  anchor3 = solstice_parser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&anchor3->name), "entity0b"), 0);
  CHECK(d3_eq(anchor3->position, d3(tmp, 7, 8, 9)), 1);

  anchor4 = solstice_parser_find_anchor(parser, "entity0");
  CHECK(anchor4, NULL);
  anchor4 = solstice_parser_find_anchor(parser, "entity0.anchor0");
  CHECK(anchor4, anchor0);
  anchor4 = solstice_parser_find_anchor(parser, "entity0.anchor1");
  CHECK(anchor4, anchor1);
  anchor4 = solstice_parser_find_anchor(parser, "entity0.entity0a.anchor0");
  CHECK(anchor4, NULL);
  anchor4 = solstice_parser_find_anchor(parser, "entity0.entity0b.anchor0");
  CHECK(anchor4, anchor2);
  anchor4 = solstice_parser_find_anchor(parser, "entity0.entity0b.entity0b");
  CHECK(anchor4, anchor3);
  anchor4 = solstice_parser_find_anchor(parser, "entity1.entity0b.anchor1");
  CHECK(anchor4, NULL);

  solstice_entity_iterator_next(&it);
  CHECK(solstice_entity_iterator_eq(&it, &it_end), 1);

  CHECK(solstice_parser_load(parser), RES_BAD_OP);
  solstice_parser_ref_put(parser);
  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
