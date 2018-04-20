/* Copyright (C) 2016-2018 CNRS
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
  struct solparser_geometry_iterator geom_it, geom_it_end;
  struct solparser_material_iterator mtl_it, mtl_it_end;
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  struct solparser_geometry_id geom_id;
  const struct solparser_geometry* geoms[2] = { NULL, NULL };
  const struct solparser_material* mtls[2] = { NULL, NULL };
  const struct solparser_entity* entity, *entity1a, *entity1b, *entity2, *entity3;
  const struct solparser_geometry* geom;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_matte* matte;
  const struct solparser_material_mirror* mirror;
  const struct solparser_shape_sphere* sphere;
  const struct solparser_sun* sun;
  const struct solparser_spectrum* spectrum;
  size_t nmtls = 0;
  size_t ngeoms = 0;
  double tmp[3];
  long fp;
  FILE* stream;
  (void)argc, (void)argv;

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  solparser_create(&allocator, &parser);

  stream = tmpfile();
  CHK(stream != NULL);

  fprintf(stream, "- geometry: &sphere\n");
  fprintf(stream, "    - sphere: { radius: 1  }\n");
  fprintf(stream, "      material: { matte: { reflectivity: 1 } }\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry: *sphere\n");

  fp = ftell(stream);
  fprintf(stream, "    name: invalid name\n");
  rewind(stream);
  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_BAD_ARG);

  CHK(fseek(stream, fp, SEEK_SET) != -1);
  fprintf(stream, "    name: invalid\tname\n");
  rewind(stream);
  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_BAD_ARG);

  CHK(fseek(stream, fp, SEEK_SET) != -1);
  fprintf(stream, "    name: invalid.name\n");
  rewind(stream);
  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_BAD_ARG);

  CHK(fseek(stream, fp, SEEK_SET) != -1);
  fprintf(stream, "    name: \t\t\t lvl0  \t \n");
  fprintf(stream, "    transform: { translation: [1,2,3], rotation: [4,5,6]}\n");
  fprintf(stream, "    children:\n");
  fprintf(stream, "      - name: lvl1a\n");
  fprintf(stream, "        primary: 1\n");
  fprintf(stream, "        geometry: \n");
  fprintf(stream, "          - sphere: {radius: 2}\n");
  fprintf(stream, "            material:\n");
  fprintf(stream, "              mirror: { reflectivity: 0.9, slope_error: 0.1 }\n");
  fprintf(stream, "      - name: lvl1b\n");
  fprintf(stream, "        primary: 0\n");
  fprintf(stream, "        geometry: *sphere\n");
  fprintf(stream, "        transform: { rotation: [3.14, 0, -1] }\n");
  fprintf(stream, "        children:\n");
  fprintf(stream, "          - name: lvl2\n");
  fprintf(stream, "            primary: 0\n");
  fprintf(stream, "            geometry: *sphere\n");
  fprintf(stream, "- sun:\n");
  fprintf(stream, "    dni: 1\n");
  fprintf(stream, "    spectrum: [ { wavelength: 1, data: 1} ]\n");
  fprintf(stream, "- atmosphere:\n");
  fprintf(stream, "    extinction: 0\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_geometry_iterator_begin(parser, &geom_it);
  solparser_geometry_iterator_end(parser, &geom_it_end);
  ngeoms = 0;
  while(!solparser_geometry_iterator_eq(&geom_it, &geom_it_end)) {
    CHK(ngeoms < 2);
    geom_id = solparser_geometry_iterator_get(&geom_it);
    solparser_geometry_iterator_next(&geom_it);
    geoms[ngeoms] = solparser_get_geometry(parser, geom_id);
    ++ngeoms;
  }
  CHK(ngeoms == 2);

  solparser_material_iterator_begin(parser, &mtl_it);
  solparser_material_iterator_end(parser, &mtl_it_end);
  nmtls = 0;
  while(!solparser_material_iterator_eq(&mtl_it, &mtl_it_end)) {
    struct solparser_material_id mtl_id;
    CHK(nmtls < 2);
    mtl_id = solparser_material_iterator_get(&mtl_it);
    solparser_material_iterator_next(&mtl_it);
    mtls[nmtls] = solparser_get_material(parser, mtl_id);
    ++nmtls;
  }
  CHK(nmtls == 2);

  solparser_entity_iterator_begin(parser, &it);
  solparser_entity_iterator_end(parser, &end);
  CHK(solparser_entity_iterator_eq(&it, &end) == 0);

  entity_id = solparser_entity_iterator_get(&it);
  entity = solparser_get_entity(parser, entity_id);

  CHK(strcmp("lvl0", str_cget(&entity->name)) == 0);
  CHK(solparser_entity_get_children_count(entity) == 2);
  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  geom_id = entity->data.geometry;
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHK(geom == geoms[0] || geom == geoms[1]);
  CHK(solparser_geometry_get_objects_count(geom) == 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  CHK(d3_eq(obj->rotation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(obj->translation, d3_splat(tmp, 0)) == 1);
  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_SPHERE);
  sphere = solparser_get_shape_sphere(parser, shape->data.sphere);
  CHK(sphere->radius == 1);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i == mtl2->back.i);
  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl == mtls[0] || mtl == mtls[1]);
  CHK(mtl->type == SOLPARSER_MATERIAL_MATTE);
  matte = solparser_get_material_matte(parser, mtl->data.matte);
  CHK(matte->reflectivity.type == SOLPARSER_MTL_DATA_REAL);
  CHK(matte->reflectivity.value.real == 1);

  entity_id = solparser_entity_get_child(entity, 0);
  entity1a = solparser_get_entity(parser, entity_id);
  CHK(d3_eq(entity1a->translation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(entity1a->rotation, d3_splat(tmp, 0)) == 1);
  CHK(strcmp("lvl1a", str_cget(&entity1a->name)) == 0);
  CHK(solparser_entity_get_children_count(entity1a) == 0);
  CHK(entity1a->type == SOLPARSER_ENTITY_GEOMETRY);
  CHK(entity1a->primary == 1);
  CHK(entity1a->data.geometry.i != geom_id.i);
  geom = solparser_get_geometry(parser, entity1a->data.geometry);
  CHK(geom == geoms[0] || geom == geoms[1]);
  CHK(solparser_geometry_get_objects_count(geom) == 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  CHK(d3_eq(obj->rotation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(obj->translation, d3_splat(tmp, 0)) == 1);
  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_SPHERE);
  sphere = solparser_get_shape_sphere(parser, shape->data.sphere);
  CHK(sphere->radius == 2);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i == mtl2->back.i);
  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl == mtls[0] || mtl == mtls[1]);
  CHK(mtl->type == SOLPARSER_MATERIAL_MIRROR);
  mirror = solparser_get_material_mirror(parser, mtl->data.mirror);
  CHK(mirror->reflectivity.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->reflectivity.value.real == 0.9);
  CHK(mirror->slope_error.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mirror->slope_error.value.real == 0.1);

  entity_id = solparser_entity_get_child(entity, 1);
  entity1b = solparser_get_entity(parser, entity_id);
  CHK(d3_eq(entity1b->translation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq_eps(entity1b->rotation, d3(tmp, 3.14, 0, -1), 1.e-6) == 1);
  CHK(strcmp("lvl1b", str_cget(&entity1b->name)) == 0);
  CHK(solparser_entity_get_children_count(entity1b) == 1);
  CHK(entity1b->type == SOLPARSER_ENTITY_GEOMETRY);
  CHK(entity1b->primary == 0);
  CHK(entity1b->data.geometry.i == geom_id.i);

  entity_id = solparser_entity_get_child(entity1b, 0);
  entity2 = solparser_get_entity(parser, entity_id);
  CHK(d3_eq(entity2->translation, d3_splat(tmp, 0)) == 1);
  CHK(d3_eq(entity2->rotation, d3_splat(tmp, 0)) == 1);
  CHK(strcmp("lvl2", str_cget(&entity2->name)) == 0);
  CHK(solparser_entity_get_children_count(entity2) == 0);
  CHK(entity2->type == SOLPARSER_ENTITY_GEOMETRY);
  CHK(entity2->data.geometry.i == geom_id.i);

  entity3 = solparser_find_entity(parser, "lvl0");
  CHK(entity3 == entity);
  entity3 = solparser_find_entity(parser, "lvl1a");
  CHK(entity3 == NULL);
  entity3 = solparser_find_entity(parser, "lvl0.lvl1a");
  CHK(entity3 == entity1a);
  entity3 = solparser_find_entity(parser, "lvl0.lvl1b");
  CHK(entity3 == entity1b);
  entity3 = solparser_find_entity(parser, "lvl0.lvl1b.lvl2");
  CHK(entity3 == entity2);
  entity3 = solparser_find_entity(parser,"lvl0.lvl1b.bad_name");
  CHK(entity3 == NULL);

  sun = solparser_get_sun(parser);
  CHK(sun != NULL);
  CHK(sun->dni == 1.0);
  CHK(sun->radang_distrib_type == SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL);
  CHK(SOLPARSER_ID_IS_VALID(sun->spectrum) == 1);
  spectrum = solparser_get_spectrum(parser, sun->spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 1.0);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 1.0);

  CHK(solparser_load(parser) == RES_BAD_OP);
  solparser_ref_put(parser);

  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}

