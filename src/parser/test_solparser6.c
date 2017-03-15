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

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
  struct solparser_entity_iterator it, end;
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  const struct solparser_entity* entity;
  const struct solparser_geometry* geom;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  const struct solparser_shape_sphere* sphere;
  const struct solparser_shape_paraboloid* parabol;
  const struct solparser_shape_hyperboloid* hyperbol;
  FILE* stream;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  solparser_create(&allocator, &parser);

  stream = tmpfile();
  NCHECK(stream, NULL);

  fprintf(stream, "- sun: { dni: 1, spectrum: [{wavelength: 1, data: 1 }] }\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: test\n");
  fprintf(stream, "    primary: 0\n"); 
  fprintf(stream, "    geometry:\n");
  fprintf(stream, "    - sphere: { radius: 1 }\n");
  fprintf(stream, "      material: { ?virtual }\n");
  fprintf(stream, "    - parabol:\n");
  fprintf(stream, "        focal: 10\n");
  fprintf(stream, "        slices : 10\n");
  fprintf(stream, "        clip :\n");
  fprintf(stream, "        - operation : AND\n");
  fprintf(stream, "          vertices : [[1, 2], [3, 4], [6, 7]]\n");
  fprintf(stream, "      material: { ?virtual }\n");
  fprintf(stream, "    - hyperbol:\n");
  fprintf(stream, "        focals: { real: 10, image: 2 }\n");
  fprintf(stream, "        slices : 20\n");
  fprintf(stream, "        clip :\n");
  fprintf(stream, "          - operation : AND\n");
  fprintf(stream, "            vertices : [[1, 2], [3, 4], [6, 7]]\n");
  fprintf(stream, "      material: { ?virtual }\n");
  rewind(stream);

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
  CHECK(solparser_geometry_get_objects_count(geom), 3);

  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLPARSER_SHAPE_SPHERE);
  sphere = solparser_get_shape_sphere(parser, shape->data.sphere);
  CHECK(sphere->radius, 1);
  CHECK(sphere->nslices, 16);

  obj_id = solparser_geometry_get_object(geom, 1);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLPARSER_SHAPE_PARABOL);
  parabol = solparser_get_shape_parabol(parser, shape->data.parabol);
  CHECK(parabol->focal, 10);

  obj_id = solparser_geometry_get_object(geom, 2);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHECK(shape->type, SOLPARSER_SHAPE_HYPERBOL);
  hyperbol = solparser_get_shape_hyperbol(parser, shape->data.hyperbol);
  CHECK(hyperbol->focals.real, 10);
  CHECK(hyperbol->focals.image, 2);

  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHECK(mtl2->front.i, mtl2->back.i);
  mtl = solparser_get_material(parser, mtl2->front);
  CHECK(mtl->type, SOLPARSER_MATERIAL_VIRTUAL);

  solparser_entity_iterator_next(&it);
  CHECK(solparser_entity_iterator_eq(&it, &end), 1);

  CHECK(solparser_load(parser), RES_BAD_OP);
  solparser_ref_put(parser);

  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}

