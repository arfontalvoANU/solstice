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
  const struct solparser_medium* vacuum;
  const struct solparser_medium* glass;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_dielectric* dielec;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  const struct solparser_spectrum* spectrum;
  FILE* stream;
  (void)argc, (void)argv;

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  CHK(solparser_create(&allocator, &parser) == RES_OK);

  stream = tmpfile();
  CHK(stream != NULL);

  fprintf(stream, "- sun: { dni: 1, spectrum: [{wavelength: 1, data: 1 }] }\n");
  fprintf(stream, "- medium: &vacuum {refractive_index: 1, extinction: 0}\n");
  fprintf(stream, "- medium: &glass \n");
  fprintf(stream, "    refractive_index: 1.5\n");
  fprintf(stream, "    extinction: \n");
  fprintf(stream, "    - {wavelength: 1, data: 21}\n");
  fprintf(stream, "    - {wavelength: 2, data: 22}\n");
  fprintf(stream, "    - {wavelength: 3, data: 23}\n");
  fprintf(stream, "    - {wavelength: 4, data: 24}\n");
  fprintf(stream, "    - {wavelength: 5, data: 25}\n");
  fprintf(stream, "    - {wavelength: 6, data: 26}\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: test\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry:\n");
  fprintf(stream, "      - sphere: { radius: 1 }\n");
  fprintf(stream, "        material:\n");
  fprintf(stream, "          front:\n");
  fprintf(stream, "            dielectric:\n");
  fprintf(stream, "              medium_i: *vacuum\n");
  fprintf(stream, "              medium_t: *glass\n");
  fprintf(stream, "          back:\n");
  fprintf(stream, "            dielectric:\n");
  fprintf(stream, "              medium_i: *glass\n");
  fprintf(stream, "              medium_t: *vacuum\n");
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
  CHK(entity->type == SOLPARSER_ENTITY_GEOMETRY);
  geom = solparser_get_geometry(parser, entity->data.geometry);
  CHK(solparser_geometry_get_objects_count(geom) == 1);
  obj_id = solparser_geometry_get_object(geom, 0);
  obj = solparser_get_object(parser, obj_id);
  shape = solparser_get_shape(parser, obj->shape);
  CHK(shape->type == SOLPARSER_SHAPE_SPHERE);
  mtl2 = solparser_get_material_double_sided(parser, obj->mtl2);
  CHK(mtl2->front.i != mtl2->back.i);

  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_DIELECTRIC);
  dielec = solparser_get_material_dielectric(parser, mtl->data.dielectric);
  vacuum = solparser_get_medium(parser, dielec->medium_i);
  CHK(vacuum->refractive_index.type == SOLPARSER_MTL_DATA_REAL);
  CHK(vacuum->refractive_index.value.real == 1);
  CHK(vacuum->extinction.type == SOLPARSER_MTL_DATA_REAL);
  CHK(vacuum->extinction.value.real == 0);

  glass = solparser_get_medium(parser, dielec->medium_t);
  CHK(glass->refractive_index.type == SOLPARSER_MTL_DATA_REAL);
  CHK(glass->refractive_index.value.real == 1.5);
  CHK(glass->extinction.type == SOLPARSER_MTL_DATA_SPECTRUM);
  spectrum = solparser_get_spectrum(parser, glass->extinction.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 6);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].wavelength == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[3].wavelength == 4);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[4].wavelength == 5);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[5].wavelength == 6);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 21);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 22);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].data == 23);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[3].data == 24);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[4].data == 25);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[5].data == 26);

  mtl = solparser_get_material(parser, mtl2->back);
  CHK(mtl->type == SOLPARSER_MATERIAL_DIELECTRIC);
  dielec = solparser_get_material_dielectric(parser, mtl->data.dielectric);
  CHK(solparser_get_medium(parser, dielec->medium_i) == glass);
  CHK(solparser_get_medium(parser, dielec->medium_t) == vacuum);

  CHK(solparser_load(parser) == RES_BAD_OP);
  solparser_ref_put(parser);

  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}

