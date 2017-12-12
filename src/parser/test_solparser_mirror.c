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

static void
check_object
  (struct solparser* parser,
   const struct solparser_object* obj,
   const double reflectivity,
   const double roughness,
   const enum solparser_microfacet_distribution distrib)
{
  const struct solparser_shape* shape;
  const struct solparser_shape_sphere* sphere;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_mirror* mirror;

  CHK(obj != NULL);

  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_SPHERE);
  sphere = solparser_get_shape_sphere(parser, shape->data.sphere);
  CHK(sphere->radius == 1);

  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i == mtl2->back.i);
  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_MIRROR);
  mirror = solparser_get_material_mirror(parser, mtl->data.mirror);
  CHK(mirror->reflectivity.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->reflectivity.value.real == reflectivity);
  CHK(mirror->roughness.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->roughness.value.real == roughness);
  CHK(mirror->ufacet_distrib == distrib);
}

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
  struct solparser_entity_iterator it, it_end;
  struct solparser_entity_id entity_id;
  const struct solparser_entity* entity;
  const struct solparser_geometry* geom;
  const struct solparser_object* obj[3];
  FILE* stream;
  (void)argc, (void)argv;

  CHK((stream = tmpfile()) != NULL);
  fprintf(stream, "- sun: {dni: 1}\n");
  fprintf(stream, "- material: &specular\n");
  fprintf(stream, "    mirror:\n");
  fprintf(stream, "      reflectivity: 1\n");
  fprintf(stream, "      roughness: 0\n");
  fprintf(stream, "- material: &beckmann\n");
  fprintf(stream, "    mirror:\n");
  fprintf(stream, "      reflectivity: 0.5\n");
  fprintf(stream, "      roughness: 0.5\n");
  fprintf(stream, "      microfacet: BECKMANN\n");
  fprintf(stream, "- material: &pillbox\n");
  fprintf(stream, "    mirror:\n");
  fprintf(stream, "      reflectivity: 0.2\n");
  fprintf(stream, "      roughness: 0.2\n");
  fprintf(stream, "      microfacet: PILLBOX\n");
  fprintf(stream, "\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: entity\n");
  fprintf(stream, "    primary: 1\n");
  fprintf(stream, "    geometry: \n");
  fprintf(stream, "    - sphere: {radius: 1}\n");
  fprintf(stream, "      material: *specular\n");
  fprintf(stream, "    - sphere: {radius: 1}\n");
  fprintf(stream, "      material: *beckmann\n");
  fprintf(stream, "    - sphere: {radius: 1}\n");
  fprintf(stream, "      material: *pillbox\n");
  rewind(stream);

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  solparser_create(&allocator, &parser);
  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &it_end);
  CHK(!solparser_entity_iterator_eq(&it, &it_end));

  entity_id = solparser_entity_iterator_get(&it);
  entity = solparser_get_entity(parser, entity_id);
  CHK(!strcmp(str_cget(&entity->name), "entity"));
  CHK(entity->primary == 1);
  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHK(solparser_geometry_get_objects_count(geom) == 3);

  obj[0] = solparser_get_object(parser, solparser_geometry_get_object(geom,0));
  obj[1] = solparser_get_object(parser, solparser_geometry_get_object(geom,1));
  obj[2] = solparser_get_object(parser, solparser_geometry_get_object(geom,2));
  check_object(parser, obj[0], 1.0, 0.0, SOLPARSER_MICROFACET_BECKMANN);
  check_object(parser, obj[1], 0.5, 0.5, SOLPARSER_MICROFACET_BECKMANN);
  check_object(parser, obj[2], 0.2, 0.2, SOLPARSER_MICROFACET_PILLBOX);

  solparser_entity_iterator_next(&it);
  CHK(solparser_entity_iterator_eq(&it, &it_end));
  CHK(solparser_load(parser) == RES_BAD_OP);
  solparser_ref_put(parser);

  fclose(stream);
  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}
