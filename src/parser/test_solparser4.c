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

#include "solparser.h"
#include "test_solstice_utils.h"

const struct solparser_geometry* geometry;

static const char* input[] = {
  "- sun: \n",
  "    dni: 1\n",
  "    spectrum: [{wavelength: 1, data: 1}]\n",
  "- material: &lambertian\n",
  "    mirror: { reflectivity: 0.2, slope_error: 0.1 }\n",
  "- geometry: &cuboid\n",
  "    - cuboid: { size: [1, 2, 3] }\n",
  "      material: *lambertian\n",
  "- template: &template\n",
  "    name: template0\n",
  "    primary: 1\n",
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
  (struct solparser* parser, const struct solparser_entity* entity0)
{
  const struct solparser_entity* entity;
  struct solparser_entity_id entity_id;
  double tmp[3];

  CHK(parser != NULL);
  CHK(entity0 != NULL);

  CHK(strcmp(str_cget(&entity0->name), "entity0") == 0);
  CHK(d3_eq(entity0->rotation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(entity0->translation, d3(tmp, 1, 2, 3)) == 1);
  CHK(entity0->type == SOLPARSER_ENTITY_EMPTY);

  CHK(solparser_entity_get_children_count(entity0) == 1);
  CHK(solparser_entity_get_anchors_count(entity0) == 0);

  entity_id = solparser_entity_get_child(entity0, 0);
  entity = solparser_get_entity(parser, entity_id);
  CHK(strcmp(str_cget(&entity->name), "template0") == 0);
  CHK(d3_eq(entity->translation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(entity->rotation, d3_splat(tmp, 0)) == 1);

  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  CHK(solparser_get_geometry(parser, entity->data.geometry) == geometry);
  CHK(solparser_find_entity(parser, "entity0.template0") == entity);
}

static void
check_entity1
  (struct solparser* parser, const struct solparser_entity* entity1)
{
  const struct solparser_entity* entity;
  struct solparser_entity_id entity_id;
  double tmp[3];

  CHK(parser != NULL);
  CHK(entity1 != NULL);

  CHK(strcmp(str_cget(&entity1->name), "entity1") == 0);
  CHK(d3_eq(entity1->rotation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(entity1->translation, d3(tmp, 3, 4, 5)) == 1);
  CHK(entity1->type == SOLPARSER_ENTITY_EMPTY);

  CHK(solparser_entity_get_children_count(entity1) == 1);
  CHK(solparser_entity_get_anchors_count(entity1) == 0);

  entity_id = solparser_entity_get_child(entity1, 0);
  entity = solparser_get_entity(parser, entity_id);
  CHK(strcmp(str_cget(&entity->name), "template0") == 0);
  CHK(d3_eq(entity->rotation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(entity->translation, d3_splat(tmp, 0)) == 1);

  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  CHK(solparser_get_geometry(parser, entity->data.geometry) == geometry);
  CHK(solparser_find_entity(parser, "entity1.template0") == entity);
}

int
main(int argc, char** argv)
{
  struct solparser_entity_iterator it, it_end;
  struct solparser_geometry_iterator it_geom, it_end_geom;
  struct mem_allocator allocator;
  struct solparser* parser;
  const struct solparser_material* mtl;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material_mirror* mirror;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  const struct solparser_shape_cuboid* cuboid;
  struct solparser_geometry_id geom_id;
  struct solparser_object_id obj_id;
  double tmp[3];
  FILE* stream;
  size_t i;
  int entity0 = 0;
  int entity1 = 0;
  (void)argc, (void)argv;

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  solparser_create(&allocator, &parser);

  stream = tmpfile();
  CHK(stream != NULL);
  i = 0;
  while(input[i]) {
    const size_t len = strlen(input[i]);
    CHK(fwrite(input[i], 1, len, stream) == len);
    ++i;
  }
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_geometry_iterator_begin(parser, &it_geom);
  solparser_geometry_iterator_end(parser, &it_end_geom);
  CHK(solparser_geometry_iterator_eq(&it_geom, &it_end_geom) == 0);
  geom_id = solparser_geometry_iterator_get(&it_geom);
  geometry = solparser_get_geometry(parser, geom_id);
  solparser_geometry_iterator_next(&it_geom);
  CHK(solparser_geometry_iterator_eq(&it_geom, &it_end_geom) == 1);

  CHK(solparser_geometry_get_objects_count(geometry) == 1);
  obj_id = solparser_geometry_get_object(geometry, 0);
  obj = solparser_get_object(parser, obj_id);
  CHK(d3_eq(obj->rotation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(obj->translation, d3_splat(tmp, 0)) == 1);

  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i == mtl2->back.i);
  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_MIRROR);
  mirror = solparser_get_material_mirror(parser, mtl->data.mirror);
  CHK(mirror->reflectivity.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->reflectivity.value.real == 0.2);
  CHK(mirror->slope_error.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->slope_error.value.real == 0.1);
  CHK(mirror->ufacet_distrib == SOLPARSER_MICROFACET_BECKMANN);

  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_CUBOID);
  cuboid = solparser_get_shape_cuboid(parser, shape->data.cuboid);
  CHK(cuboid->size[0] == 1);
  CHK(cuboid->size[1] == 2);
  CHK(cuboid->size[2] == 3);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &it_end);
  CHK(solparser_entity_iterator_eq(&it, &it_end) == 0);

  while(!solparser_entity_iterator_eq(&it, &it_end)) {
    struct solparser_entity_id entity_id;
    const struct solparser_entity* entity;

    entity_id = solparser_entity_iterator_get(&it);
    entity = solparser_get_entity(parser, entity_id);

    if(!strcmp(str_cget(&entity->name), "entity0")) {
      CHK(entity0 == 0);
      entity0 = 1;
      check_entity0(parser, entity);
    } else if(!strcmp(str_cget(&entity->name), "entity1")) {
      CHK(entity1 == 0);
      entity1 = 1;
      check_entity1(parser, entity);
    } else {
      FATAL("Unexpected entity name.\n");
    }

    solparser_entity_iterator_next(&it);
  }
  CHK(entity0 == 1);
  CHK(entity1 == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  solparser_ref_put(parser);
  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}

