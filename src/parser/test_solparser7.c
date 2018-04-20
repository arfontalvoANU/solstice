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
  struct solparser_entity_id entity_id;
  struct solparser_object_id obj_id;
  const struct solparser_entity* entity;
  const struct solparser_geometry* geom;
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_material* mtl;
  const struct solparser_material_thin_dielectric* thin;
  const struct solparser_medium* medium;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  const struct solparser_spectrum* spectrum;
  FILE* stream;
  (void)argc, (void)argv;

  CHK(mem_init_proxy_allocator(&allocator, &mem_default_allocator) == RES_OK);
  solparser_create(&allocator, &parser);

  stream = tmpfile();
  CHK(stream != NULL);

  fprintf(stream, "- sun: { dni: 1, spectrum: [{wavelength: 1, data: 1 }] }\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: test\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry:\n");
  fprintf(stream, "      - sphere: { radius: 1 }\n");
  fprintf(stream, "        material:\n");
  fprintf(stream, "          thin_dielectric:\n");
  fprintf(stream, "            thickness: 0.123\n");
  fprintf(stream, "            medium_i: &outside\n");
  fprintf(stream, "              refractive_index: 1\n");
  fprintf(stream, "              extinction: 0\n");
  fprintf(stream, "            medium_t: &inside\n");
  fprintf(stream, "              refractive_index: \n");
  fprintf(stream, "              - {wavelength: 1.2, data: 2.3}\n");
  fprintf(stream, "              - {wavelength: 4.5, data: 6.7}\n");
  fprintf(stream, "              - {wavelength: 0.5, data: 0.25}\n");
  fprintf(stream, "              extinction:\n");
  fprintf(stream, "              - {wavelength: 3, data: 3}\n");
  fprintf(stream, "              - {wavelength: 1, data: 1}\n");
  fprintf(stream, "              - {wavelength: 5, data: 5}\n");
  fprintf(stream, "              - {wavelength: 4, data: 4}\n");
  fprintf(stream, "              - {wavelength: 2, data: 2}\n");
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
  CHK(mtl2->front.i == mtl2->back.i);
  mtl = solparser_get_material(parser, mtl2->front);
  CHK(mtl->type == SOLPARSER_MATERIAL_THIN_DIELECTRIC);
  thin = solparser_get_material_thin_dielectric
    (parser, mtl->data.thin_dielectric);
  CHK(thin->thickness == 0.123);

  medium = solparser_get_medium(parser, thin->medium_i);
  CHK(medium->refractive_index.type == SOLPARSER_MTL_DATA_REAL);
  CHK(medium->refractive_index.value.real == 1);
  CHK(medium->extinction.type == SOLPARSER_MTL_DATA_REAL);
  CHK(medium->extinction.value.real == 0);
  medium = solparser_get_medium(parser, thin->medium_t);

  CHK(medium->refractive_index.type == SOLPARSER_MTL_DATA_SPECTRUM);
  spectrum = solparser_get_spectrum(parser, medium->refractive_index.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 0.5);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 1.2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].wavelength == 4.5);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 0.25);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 2.3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].data == 6.7);

  CHK(medium->extinction.type == SOLPARSER_MTL_DATA_SPECTRUM);
  spectrum = solparser_get_spectrum(parser, medium->extinction.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 5);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].wavelength == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[3].wavelength == 4);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[4].wavelength == 5);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].data == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[3].data == 4);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[4].data == 5);

  CHK(solparser_load(parser) == RES_BAD_OP);
  solparser_ref_put(parser);

  fclose(stream);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}
