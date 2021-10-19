/* Copyright (C) 2018, 2019, 2021 |Meso|Star> (contact@meso-star.com)
 * Copyright (C) 2016-2018 CNRS
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

static void
test_dielectric(struct solparser* parser)
{
  struct solparser_entity_iterator it, end;
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  const struct solparser_entity* entity;
  const struct solparser_image* img;
  const struct solparser_geometry* geom;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_dielectric* dielectric;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  FILE* stream;

  stream = tmpfile();
  CHK(stream != NULL);

  fprintf(stream, "- sun: {dni: 1, spectrum: [{wavelength: 1, data: 1} ]}\n");
  fprintf(stream, "\n");
  fprintf(stream, "- material: &glass\n");
  fprintf(stream, "    front:\n");
  fprintf(stream, "      dielectric:\n");
  fprintf(stream, "        medium_i: &out {refractive_index: 1, extinction: 0}\n");
  fprintf(stream, "        medium_t: &in  {refractive_index: 1.5, extinction: 20}\n");
  fprintf(stream, "        normal_map: {path: my_normal_map}\n");
  fprintf(stream, "    back: {dielectric: {medium_i: *in, medium_t: *out}}\n");
  fprintf(stream, "\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: foo\n");
  fprintf(stream, "    primary: 1\n");
  fprintf(stream, "    geometry:\n");
  fprintf(stream, "    - cylinder: { radius: 2, height: 2 }\n");
  fprintf(stream, "      material: *glass\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &end);
  CHK(solparser_entity_iterator_eq(&it, &end) == 0);

  entity_id = solparser_entity_iterator_get(&it);
  entity = solparser_get_entity(parser, entity_id);

  CHK(strcmp("foo",  str_cget(&entity->name)) == 0);
  CHK(solparser_entity_get_children_count(entity) == 0);
  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  CHK(entity->primary == 1);
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHK(solparser_geometry_get_objects_count(geom) == 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_CYLINDER);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i != mtl2->back.i);

  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_DIELECTRIC);
  dielectric = solparser_get_material_dielectric(parser, mtl->data.dielectric);
  CHK(SOLPARSER_ID_IS_VALID(dielectric->normal_map) == 1);
  img = solparser_get_image(parser, dielectric->normal_map);
  CHK(strcmp(str_cget(&img->filename), "my_normal_map") == 0);

  mtl = solparser_get_material(parser, mtl2->back);
  CHK(mtl->type == SOLPARSER_MATERIAL_DIELECTRIC);
  dielectric = solparser_get_material_dielectric(parser, mtl->data.dielectric);
  CHK(SOLPARSER_ID_IS_VALID(dielectric->normal_map) == 0);

  solparser_entity_iterator_next(&it);
  CHK(solparser_entity_iterator_eq(&it, &end) == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

static void
test_matte(struct solparser* parser)
{
  struct solparser_entity_iterator it, end;
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  const struct solparser_entity* entity;
  const struct solparser_image* img;
  const struct solparser_geometry* geom;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_matte* matte;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  FILE* stream;

  stream = tmpfile();
  CHK(stream != NULL);

  fprintf(stream, "- sun: { dni: 1, spectrum: [{wavelength: 1, data: 1} ] }\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: test\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry:\n");
  fprintf(stream, "      - sphere: { radius: 1 }\n");
  fprintf(stream, "        material:\n");
  fprintf(stream, "          matte:\n");
  fprintf(stream, "            reflectivity: 0.123\n");
  fprintf(stream, "            normal_map: { path: \"path to normal map\" }\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &end);
  CHK(solparser_entity_iterator_eq(&it, &end) == 0);

  entity_id = solparser_entity_iterator_get(&it);
  entity = solparser_get_entity(parser, entity_id);

  CHK(strcmp("test",  str_cget(&entity->name)) == 0);
  CHK(solparser_entity_get_children_count(entity) == 0);
  CHK(entity->primary == 0);
  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHK(solparser_geometry_get_objects_count(geom) == 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_SPHERE);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i == mtl2->back.i);

  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_MATTE);
  matte = solparser_get_material_matte(parser, mtl->data.matte);
  CHK(matte->reflectivity.type == SOLPARSER_MTL_DATA_REAL);
  CHK(matte->reflectivity.value.real == 0.123);
  CHK(SOLPARSER_ID_IS_VALID(matte->normal_map) == 1);
  img = solparser_get_image(parser, matte->normal_map);
  CHK(strcmp(str_cget(&img->filename), "path to normal map") == 0);

  solparser_entity_iterator_next(&it);
  CHK(solparser_entity_iterator_eq(&it, &end) == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

static void
test_mirror(struct solparser* parser)
{
  struct solparser_entity_iterator it, end;
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  const struct solparser_entity* entity;
  const struct solparser_image* img;
  const struct solparser_geometry* geom;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_mirror* mirror;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  FILE* stream;

  stream = tmpfile();
  CHK(stream != NULL);

  fprintf(stream, "- sun: { dni: 1, spectrum: [{wavelength: 1, data: 1} ] }\n");
  fprintf(stream, "- entity: \n");
  fprintf(stream, "    name: my_entity\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry: \n");
  fprintf(stream, "      - cuboid: { size: [1, 1, 1] }\n");
  fprintf(stream, "        material: \n");
  fprintf(stream, "          mirror:\n");
  fprintf(stream, "            reflectivity: 1\n");
  fprintf(stream, "            slope_error: 0.1\n");
  fprintf(stream, "            normal_map: { path: Normal map } \n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &end);
  CHK(solparser_entity_iterator_eq(&it, &end) == 0);

  entity_id = solparser_entity_iterator_get(&it);
  entity = solparser_get_entity(parser, entity_id);

  CHK(strcmp("my_entity",  str_cget(&entity->name)) == 0);
  CHK(solparser_entity_get_children_count(entity) == 0);
  CHK(entity->primary == 0);
  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHK(solparser_geometry_get_objects_count(geom) == 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_CUBOID);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i == mtl2->back.i);

  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_MIRROR);
  mirror = solparser_get_material_mirror(parser, mtl->data.mirror);
  CHK(mirror->reflectivity.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->reflectivity.value.real == 1);
  CHK(mirror->slope_error.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->slope_error.value.real == 0.1);
  CHK(SOLPARSER_ID_IS_VALID(mirror->normal_map) == 1);
  img = solparser_get_image(parser, mirror->normal_map);
  CHK(strcmp(str_cget(&img->filename), "Normal map") == 0);

  solparser_entity_iterator_next(&it);
  CHK(solparser_entity_iterator_eq(&it, &end) == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

static void
test_thin_dielectric(struct solparser* parser)
{
  struct solparser_entity_iterator it, end;
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  const struct solparser_entity* entity;
  const struct solparser_image* img;
  const struct solparser_geometry* geom;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_thin_dielectric* thin;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  FILE* stream;

  stream = tmpfile();
  CHK(stream != NULL);
  fprintf(stream, "- sun: { dni: 1, spectrum: [{wavelength: 1, data: 1} ] }\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: test\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry:\n");
  fprintf(stream, "      - sphere: { radius: 1 }\n");
  fprintf(stream, "        material:\n");
  fprintf(stream, "          thin_dielectric:\n");
  fprintf(stream, "            thickness: 0.1\n");
  fprintf(stream, "            medium_i:\n");
  fprintf(stream, "              refractive_index: 1\n");
  fprintf(stream, "              extinction: 0\n");
  fprintf(stream, "            medium_t:\n");
  fprintf(stream, "              refractive_index: 1.5\n");
  fprintf(stream, "              extinction: 20\n");
  fprintf(stream, "            normal_map: { path: Bump }\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &end);
  CHK(solparser_entity_iterator_eq(&it, &end) == 0);

  entity_id = solparser_entity_iterator_get(&it);
  entity = solparser_get_entity(parser, entity_id);

  CHK(strcmp("test",  str_cget(&entity->name)) == 0);
  CHK(solparser_entity_get_children_count(entity) == 0);
  CHK(entity->primary == 0);
  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHK(solparser_geometry_get_objects_count(geom) == 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_SPHERE);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i == mtl2->back.i);

  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_THIN_DIELECTRIC);
  thin = solparser_get_material_thin_dielectric(parser, mtl->data.thin_dielectric);
  CHK(thin->thickness == 0.1);
  CHK(thin->medium_i.i != thin->medium_t.i);
  CHK(SOLPARSER_ID_IS_VALID(thin->normal_map) == 1);
  img = solparser_get_image(parser, thin->normal_map);
  CHK(strcmp(str_cget(&img->filename), "Bump") == 0);

  solparser_entity_iterator_next(&it);
  CHK(solparser_entity_iterator_eq(&it, &end) == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
  (void)argc, (void)argv;

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  CHK(solparser_create(&allocator, &parser) == RES_OK);

  test_dielectric(parser);
  test_matte(parser);
  test_mirror(parser);
  test_thin_dielectric(parser);

  solparser_ref_put(parser);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}

