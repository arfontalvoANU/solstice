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
#include "test_solstice_utils.h"

static const char* input[] = {
  "- material: &lambertian\n",
  "    matte: { reflectivity: 0.5 }\n",
  "- geometry: &cylinder\n",
  "    - cylinder: { radius: 1, height: 10, slices: 128 }\n",
  "      material: *lambertian\n",
  "- sun: \n",
  "    dni: 1\n",
  "    spectrum: [{wavelength: 1, data: 1}]\n",
  "- entity:\n",
  "    name: entity0\n",
  "    geometry: *cylinder\n",
  "    anchors:\n",
  "      - name: anchor0\n",
  "        position: [1, 2, 3]\n",
  "      - name: anchor1\n",
  "        position: [4, 5, 6]\n",
  "    children:\n",
  "      - name: entity0a\n",
  "        geometry: *cylinder\n",
  "      - name: entity0b\n",
  "        geometry: *cylinder\n",
  "        anchors:\n\n",
  "          - name: anchor0\n",
  "            position: [4, 5, 6]\n",
  "          - name: entity0b\n",
  "            position: [7, 8, 9]\n",
  "- entity:\n",
  "    name: entity1\n",
  "    pivot:\n",
  "      point: [1, 2, 3]\n",
  "      normal: [4, 5, 6]\n",
  "      target: { anchor: \"entity0.entity0b.anchor0\" }\n",
  NULL
};

const struct solparser_anchor* entity0_anchor0;
const struct solparser_anchor* entity0_anchor1;
const struct solparser_anchor* entity0_entity0b_anchor0;
const struct solparser_anchor* entity0_entity0b_entity0b;
const struct solparser_geometry* geom;
const struct solparser_pivot* pivot;

static void
check_entity0
  (struct solparser* parser, const struct solparser_entity* entity0)
{
  struct solparser_anchor_id anchor_id;
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  const struct solparser_entity *entity0a, *entity0b;
  const struct solparser_material_matte* matte;
  const struct solparser_material* mtl;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  const struct solparser_shape_cylinder* cylinder;
  double tmp[3];

  NCHECK(parser, NULL);
  NCHECK(entity0, NULL);

  CHECK(strcmp(str_cget(&entity0->name), "entity0"), 0);
  CHECK(d3_eq(entity0->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(entity0->translation, d3_splat(tmp, 0)), 1);
  CHECK(entity0->type, SOLPARSER_ENTITY_GEOMETRY);

  geom = solparser_get_geometry(parser, entity0->data.geometry);
  CHECK(solparser_geometry_get_objects_count(geom), 1);

  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  CHECK(d3_eq(obj->rotation, d3_splat(tmp, 0)), 1);
  CHECK(d3_eq(obj->translation, d3_splat(tmp, 0)), 1);

  shape = solparser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLPARSER_SHAPE_CYLINDER);

  cylinder = solparser_get_shape_cylinder(parser, shape->data.cylinder);
  CHECK(cylinder->height, 10);
  CHECK(cylinder->radius, 1);
  CHECK(cylinder->nslices, 128);

  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHECK(mtl2->front.i, mtl2->back.i);

  mtl = solparser_get_material(parser, mtl2->front);
  CHECK(mtl->type, SOLPARSER_MATERIAL_MATTE);

  matte = solparser_get_material_matte(parser, mtl->data.matte);
  CHECK(matte->reflectivity, 0.5);

  CHECK(solparser_entity_get_children_count(entity0), 2);
  CHECK(solparser_entity_get_anchors_count(entity0), 2);

  anchor_id = solparser_entity_get_anchor(entity0, 0);
  entity0_anchor0 = solparser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&entity0_anchor0->name), "anchor0"), 0);
  CHECK(d3_eq(entity0_anchor0->position, d3(tmp, 1, 2, 3)), 1);

  anchor_id = solparser_entity_get_anchor(entity0, 1);
  entity0_anchor1 = solparser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&entity0_anchor1->name), "anchor1"), 0);
  CHECK(d3_eq(entity0_anchor1->position, d3(tmp, 4, 5, 6)), 1);

  entity_id = solparser_entity_get_child(entity0, 0);
  entity0a = solparser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity0a->name), "entity0a"), 0);
  CHECK(entity0a->type, SOLPARSER_ENTITY_GEOMETRY);
  CHECK(entity0->data.geometry.i, entity0a->data.geometry.i);
  CHECK(solparser_entity_get_anchors_count(entity0a), 0);
  CHECK(solparser_entity_get_children_count(entity0a), 0);

  entity_id = solparser_entity_get_child(entity0, 1);
  entity0b = solparser_get_entity(parser, entity_id);
  CHECK(strcmp(str_cget(&entity0b->name), "entity0b"), 0);
  CHECK(entity0b->type, SOLPARSER_ENTITY_GEOMETRY);
  CHECK(entity0->data.geometry.i, entity0b->data.geometry.i);
  CHECK(solparser_entity_get_anchors_count(entity0b), 2);
  CHECK(solparser_entity_get_children_count(entity0b), 0);

  anchor_id = solparser_entity_get_anchor(entity0b, 0);
  entity0_entity0b_anchor0 = solparser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&entity0_entity0b_anchor0->name), "anchor0"), 0);
  CHECK(d3_eq(entity0_entity0b_anchor0->position, d3(tmp, 4, 5, 6)), 1);

  anchor_id = solparser_entity_get_anchor(entity0b, 1);
  entity0_entity0b_entity0b = solparser_get_anchor(parser, anchor_id);
  CHECK(strcmp(str_cget(&entity0_entity0b_entity0b->name), "entity0b"), 0);
  CHECK(d3_eq(entity0_entity0b_entity0b->position, d3(tmp, 7, 8, 9)), 1);
}

static void
check_entity1
  (struct solparser* parser, const struct solparser_entity* entity1)
{
  double tmp[3];

  NCHECK(parser, NULL);
  NCHECK(entity1, NULL);

  CHECK(strcmp(str_cget(&entity1->name), "entity1"), 0);
  CHECK(entity1->type, SOLPARSER_ENTITY_PIVOT);
  CHECK(solparser_entity_get_anchors_count(entity1), 0);
  CHECK(solparser_entity_get_children_count(entity1), 0);

  pivot = solparser_get_pivot(parser, entity1->data.pivot);
  CHECK(d3_eq(pivot->point, d3(tmp, 1, 2, 3)), 1);
  CHECK(d3_eq(pivot->normal, d3(tmp, 4, 5, 6)), 1);
  CHECK(pivot->target_type, SOLPARSER_TARGET_ANCHOR);
}

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
  struct solparser_entity_iterator it, it_end;
  const struct solparser_anchor* anchor;
  FILE* stream;
  size_t i;
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
    struct solparser_entity_id entity_id;
    const struct solparser_entity* entity;

    entity_id = solparser_entity_iterator_get(&it);
    entity = solparser_get_entity(parser, entity_id);

    if(!strcmp(str_cget(&entity->name), "entity0")) {
      check_entity0(parser, entity);
    } else if(!strcmp(str_cget(&entity->name), "entity1")) {
      check_entity1(parser, entity);
    } else {
      FATAL("Unexpected entity name.\n");
    }

    solparser_entity_iterator_next(&it);
  }

  anchor = solparser_get_anchor(parser, pivot->target.anchor);
  CHECK(anchor, entity0_entity0b_anchor0);

  anchor = solparser_find_anchor(parser, "entity0");
  CHECK(anchor, NULL);
  anchor = solparser_find_anchor(parser, "entity0.anchor0");
  CHECK(anchor, entity0_anchor0);
  anchor = solparser_find_anchor(parser, "entity0.anchor1");
  CHECK(anchor, entity0_anchor1);
  anchor = solparser_find_anchor(parser, "entity0.entity0a.anchor0");
  CHECK(anchor, NULL);
  anchor = solparser_find_anchor(parser, "entity0.entity0b.anchor0");
  CHECK(anchor, entity0_entity0b_anchor0);
  anchor = solparser_find_anchor(parser, "entity0.entity0b.entity0b");
  CHECK(anchor, entity0_entity0b_entity0b);
  anchor = solparser_find_anchor(parser, "entity1.entity0b.anchor1");
  CHECK(anchor, NULL);

  CHECK(solparser_load(parser), RES_BAD_OP);
  solparser_ref_put(parser);
  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
