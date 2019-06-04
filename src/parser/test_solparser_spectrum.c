/* Copyright (C) 2016-2018 CNRS, 2018-2019 |Meso|Star>
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

static void
test_sun(struct solparser* parser)
{
  const struct solparser_sun* sun;
  const struct solparser_spectrum* spectrum;
  FILE* stream;
  size_t i;

  CHK((stream = tmpfile()) != NULL);

  fprintf(stream, "- spectrum: &my_spectrum\n");
  fprintf(stream, "  - { wavelength: 2, data: 2 }\n");
  fprintf(stream, "  - { wavelength: 1, data: 1 }\n");
  fprintf(stream, "  - { wavelength: 8, data: 8 }\n");
  fprintf(stream, "  - { wavelength: 3, data: 3 }\n");
  fprintf(stream, "  - { wavelength: 5, data: 5 }\n");
  fprintf(stream, "  - { wavelength: 9, data: 9 }\n");
  fprintf(stream, "  - { wavelength: 6, data: 6 }\n");
  fprintf(stream, "  - { wavelength: 7, data: 7 }\n");
  fprintf(stream, "  - { wavelength: 4, data: 4 }\n");
  fprintf(stream, "- sun:\n");
  fprintf(stream, "    dni: 123.456\n");
  fprintf(stream, "    spectrum: *my_spectrum\n");
  fprintf(stream, "- material: &matte { matte: { reflectivity: 1 } }\n");
  fprintf(stream, "- entity:\n");
  fprintf(stream, "    name: foo-bar\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry: [{sphere: {radius: 1}, material: *matte}]\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  sun = solparser_get_sun(parser);
  CHK(sun->dni == 123.456);
  CHK(sun->radang_distrib_type == SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL);
  CHK(SOLPARSER_ID_IS_VALID(sun->spectrum) == 1);
  spectrum = solparser_get_spectrum(parser, sun->spectrum);

  CHK(darray_spectrum_data_size_get(&spectrum->data) == 9);

  FOR_EACH(i, 0, darray_spectrum_data_size_get(&spectrum->data)) {
    CHK(darray_spectrum_data_cdata_get(&spectrum->data)[i].wavelength == i+1);
    CHK(darray_spectrum_data_cdata_get(&spectrum->data)[i].wavelength == i+1);
  }

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);

  CHK((stream = tmpfile()) != NULL);
  fprintf(stream, "- sun: {dni: 1}\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  CHK(SOLPARSER_ID_IS_VALID(sun->spectrum) == 0);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

static void
test_matte(struct solparser* parser)
{
  struct solparser_material_iterator mtl_it, mtl_it_end;
  const struct solparser_material* mtl;
  const struct solparser_material_matte* matte;
  const struct solparser_spectrum* spectrum;
  FILE* stream;

  CHK((stream = tmpfile()) != NULL);

  fprintf(stream, "- sun: { dni: 1 }\n");
  fprintf(stream, "- material:\n");
  fprintf(stream, "    matte:\n");
  fprintf(stream, "      reflectivity:\n");
  fprintf(stream, "      - { wavelength: 3.4, data: 0.5 }\n");
  fprintf(stream, "      - { wavelength: 1.2, data: 0.25 }\n");
  fprintf(stream, "      - { wavelength: 6.7, data: 0.125 }\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_material_iterator_begin(parser, &mtl_it);
  solparser_material_iterator_end(parser, &mtl_it_end);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 0);

  mtl = solparser_get_material(parser, solparser_material_iterator_get(&mtl_it));
  CHK(mtl->type == SOLPARSER_MATERIAL_MATTE);
  matte = solparser_get_material_matte(parser, mtl->data.matte);
  CHK(matte->reflectivity.type == SOLPARSER_MTL_DATA_SPECTRUM);
  CHK(SOLPARSER_ID_IS_VALID(matte->normal_map) == 0);
  spectrum = solparser_get_spectrum(parser, matte->reflectivity.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 1.2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 3.4);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].wavelength == 6.7);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 0.25);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 0.5);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].data == 0.125);

  solparser_material_iterator_next(&mtl_it);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

static void
test_mirror(struct solparser* parser)
{
  struct solparser_material_iterator mtl_it, mtl_it_end;
  const struct solparser_material* mtl;
  const struct solparser_material_mirror* mirror;
  const struct solparser_spectrum* spectrum;
  FILE* stream;

  CHK((stream = tmpfile()) != NULL);

  fprintf(stream, "- sun: { dni: 1 }\n");
  fprintf(stream, "- material:\n");
  fprintf(stream, "    mirror:\n");
  fprintf(stream, "      reflectivity:\n");
  fprintf(stream, "      - { wavelength: 3.4, data: 0.5 }\n");
  fprintf(stream, "      - { wavelength: 1.2, data: 0.25 }\n");
  fprintf(stream, "      - { wavelength: 6.7, data: 0.125 }\n");
  fprintf(stream, "      slope_error:\n");
  fprintf(stream, "      - { wavelength: 123, data: 0 }\n");
  fprintf(stream, "      - { wavelength: 456, data: 1 }\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_material_iterator_begin(parser, &mtl_it);
  solparser_material_iterator_end(parser, &mtl_it_end);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 0);

  mtl = solparser_get_material(parser, solparser_material_iterator_get(&mtl_it));
  CHK(mtl->type == SOLPARSER_MATERIAL_MIRROR);
  mirror = solparser_get_material_mirror(parser, mtl->data.mirror);
  CHK(mirror->reflectivity.type == SOLPARSER_MTL_DATA_SPECTRUM);
  CHK(mirror->slope_error.type == SOLPARSER_MTL_DATA_SPECTRUM);
  CHK(SOLPARSER_ID_IS_VALID(mirror->normal_map) == 0);

  spectrum = solparser_get_spectrum(parser, mirror->reflectivity.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 1.2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 3.4);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].wavelength == 6.7);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 0.25);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 0.5);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].data == 0.125);

  spectrum = solparser_get_spectrum(parser, mirror->slope_error.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 123);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 456);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 0);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 1);

  solparser_material_iterator_next(&mtl_it);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

static void
test_thin_dielectric(struct solparser* parser)
{
  struct solparser_material_iterator mtl_it, mtl_it_end;
  const struct solparser_material* mtl;
  const struct solparser_material_thin_dielectric* thin;
  const struct solparser_medium* mdm;
  const struct solparser_spectrum* spectrum;
  FILE* stream;

  CHK((stream = tmpfile()) != NULL);

  fprintf(stream, "- sun: { dni: 1 }\n");
  fprintf(stream, "- spectrum: &refractive_index\n");
  fprintf(stream, "  - { wavelength: 123, data: 1.1 }\n");
  fprintf(stream, "  - { wavelength: 456, data: 2.2 }\n");
  fprintf(stream, "  - { wavelength: 789, data: 3.3 }\n");
  fprintf(stream, "- spectrum: &absorption\n");
  fprintf(stream, "  - { wavelength: 0.456, data: 0.2 }\n");
  fprintf(stream, "  - { wavelength: 0.123, data: 0.1 }\n");
  fprintf(stream, "- material:\n");
  fprintf(stream, "    thin_dielectric:\n");
  fprintf(stream, "      thickness: 1\n");
  fprintf(stream, "      medium_i: { refractive_index: 1, extinction: 0 }\n");
  fprintf(stream, "      medium_t: \n");
  fprintf(stream, "        refractive_index: *refractive_index\n");
  fprintf(stream, "        extinction: *absorption\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_material_iterator_begin(parser, &mtl_it);
  solparser_material_iterator_end(parser, &mtl_it_end);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 0);

  mtl = solparser_get_material(parser, solparser_material_iterator_get(&mtl_it));
  CHK(mtl->type == SOLPARSER_MATERIAL_THIN_DIELECTRIC);
  thin = solparser_get_material_thin_dielectric(parser, mtl->data.thin_dielectric);
  CHK(thin->thickness == 1);
  CHK(SOLPARSER_ID_IS_VALID(thin->normal_map) == 0);

  mdm = solparser_get_medium(parser, thin->medium_i);
  CHK(mdm->refractive_index.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mdm->refractive_index.value.real == 1);
  CHK(mdm->extinction.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mdm->extinction.value.real == 0);

  mdm = solparser_get_medium(parser, thin->medium_t);
  CHK(mdm->refractive_index.type == SOLPARSER_MTL_DATA_SPECTRUM);
  spectrum = solparser_get_spectrum(parser, mdm->refractive_index.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 123);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 456);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].wavelength == 789);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 1.1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 2.2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].data == 3.3);
  spectrum = solparser_get_spectrum(parser, mdm->extinction.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 0.123);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 0.456);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 0.1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 0.2);

  solparser_material_iterator_next(&mtl_it);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 1);

  CHK(solparser_load(parser) == RES_BAD_OP);
  fclose(stream);
}

static void
test_dielectric(struct solparser* parser)
{
  struct solparser_material_iterator mtl_it, mtl_it_end;
  const struct solparser_material* mtl;
  const struct solparser_material_dielectric* dielec;
  const struct solparser_medium* mdm;
  const struct solparser_spectrum* spectrum;
  FILE* stream;

  CHK((stream = tmpfile()) != NULL);

  fprintf(stream, "- sun: { dni: 1 }\n");
  fprintf(stream, "- spectrum: &refractive_index\n");
  fprintf(stream, "  - { wavelength: 123, data: 1.1 }\n");
  fprintf(stream, "  - { wavelength: 456, data: 2.2 }\n");
  fprintf(stream, "  - { wavelength: 789, data: 3.3 }\n");
  fprintf(stream, "- spectrum: &absorption\n");
  fprintf(stream, "  - { wavelength: 0.456, data: 0.2 }\n");
  fprintf(stream, "  - { wavelength: 0.123, data: 0.1 }\n");
  fprintf(stream, "- material:\n");
  fprintf(stream, "    dielectric:\n");
  fprintf(stream, "      medium_i: { refractive_index: 1, extinction: 0 }\n");
  fprintf(stream, "      medium_t: \n");
  fprintf(stream, "        refractive_index: *refractive_index\n");
  fprintf(stream, "        extinction: *absorption\n");
  rewind(stream);

  CHK(solparser_setup(parser, NULL, stream) == RES_OK);
  CHK(solparser_load(parser) == RES_OK);

  solparser_material_iterator_begin(parser, &mtl_it);
  solparser_material_iterator_end(parser, &mtl_it_end);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 0);

  mtl = solparser_get_material(parser, solparser_material_iterator_get(&mtl_it));
  CHK(mtl->type == SOLPARSER_MATERIAL_DIELECTRIC);
  dielec = solparser_get_material_dielectric(parser, mtl->data.dielectric);
  CHK(SOLPARSER_ID_IS_VALID(dielec->normal_map) == 0);

  mdm = solparser_get_medium(parser, dielec->medium_i);
  CHK(mdm->refractive_index.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mdm->refractive_index.value.real == 1);
  CHK(mdm->extinction.type == SOLPARSER_MTL_DATA_REAL);
  CHK(mdm->extinction.value.real == 0);

  mdm = solparser_get_medium(parser, dielec->medium_t);
  CHK(mdm->refractive_index.type == SOLPARSER_MTL_DATA_SPECTRUM);
  spectrum = solparser_get_spectrum(parser, mdm->refractive_index.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 3);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 123);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 456);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].wavelength == 789);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 1.1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 2.2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[2].data == 3.3);
  spectrum = solparser_get_spectrum(parser, mdm->extinction.value.spectrum);
  CHK(darray_spectrum_data_size_get(&spectrum->data) == 2);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].wavelength == 0.123);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].wavelength == 0.456);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[0].data == 0.1);
  CHK(darray_spectrum_data_cdata_get(&spectrum->data)[1].data == 0.2);

  solparser_material_iterator_next(&mtl_it);
  CHK(solparser_material_iterator_eq(&mtl_it, &mtl_it_end) == 1);

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

  test_sun(parser);
  test_matte(parser);
  test_mirror(parser);
  test_thin_dielectric(parser);
  test_dielectric(parser);

  solparser_ref_put(parser);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHK(mem_allocated_size() == 0);
  return 0;
}

