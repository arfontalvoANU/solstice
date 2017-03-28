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

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
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
  (void)argc, (void)argv;

  stream = tmpfile();
  NCHECK(stream, NULL);

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

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  CHECK(solparser_create(&allocator, &parser), RES_OK);
  CHECK(solparser_setup(parser, NULL, stream), RES_OK);
  CHECK(solparser_load(parser), RES_OK);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &end);
  CHECK(solparser_entity_iterator_eq(&it, &end), 0);

  entity_id = solparser_entity_iterator_get(&it);
  entity = solparser_get_entity(parser, entity_id);

  CHECK(strcmp("test",  str_cget(&entity->name)), 0);
  CHECK(solparser_entity_get_children_count(entity), 0);
  CHECK(entity->type, SOLPARSER_ENTITY_GEOMETRY);
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHECK(solparser_geometry_get_objects_count(geom), 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLPARSER_SHAPE_SPHERE);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHECK(mtl2->front.i, mtl2->back.i);

  mtl = solparser_get_material(parser, mtl2->front);
  CHECK(mtl->type, SOLPARSER_MATERIAL_MATTE);
  matte = solparser_get_material_matte(parser, mtl->data.matte);
  CHECK(matte->reflectivity, 0.123);
  CHECK(SOLPARSER_ID_IS_VALID(matte->normal_map), 1);
  img = solparser_get_image(parser, matte->normal_map);
  CHECK(strcmp(str_cget(&img->filename), "path to normal map"), 0);

  CHECK(solparser_load(parser), RES_BAD_OP);
  solparser_ref_put(parser);

  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}

