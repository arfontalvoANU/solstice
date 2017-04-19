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

static void
test_sun(struct solparser* parser)
{
  const struct solparser_sun* sun;
  const struct solparser_spectrum* spectrum;
  FILE* stream;
  size_t i;

  NCHECK(stream = tmpfile(), NULL);

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
  fprintf(stream, "    name: foo bar\n");
  fprintf(stream, "    primary: 0\n");
  fprintf(stream, "    geometry: [{sphere: {radius: 1}, material: *matte}]\n");
  rewind(stream);

  CHECK(solparser_setup(parser, NULL, stream), RES_OK);
  CHECK(solparser_load(parser), RES_OK);

  sun = solparser_get_sun(parser);
  CHECK(sun->dni, 123.456);
  CHECK(sun->radang_distrib_type, SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL);
  CHECK(SOLPARSER_ID_IS_VALID(sun->spectrum), 1);
  spectrum = solparser_get_spectrum(parser, sun->spectrum);

  CHECK(darray_spectrum_data_size_get(&spectrum->data), 9);

  FOR_EACH(i, 0, darray_spectrum_data_size_get(&spectrum->data)) {
    CHECK(darray_spectrum_data_cdata_get(&spectrum->data)[i].wavelength, i+1);
    CHECK(darray_spectrum_data_cdata_get(&spectrum->data)[i].wavelength, i+1);
  }

  CHECK(solparser_load(parser), RES_BAD_OP);
  fclose(stream);

  NCHECK(stream = tmpfile(), NULL);
  fprintf(stream, "- sun: {dni: 1}\n");
  rewind(stream);

  CHECK(solparser_setup(parser, NULL, stream), RES_OK);
  CHECK(solparser_load(parser), RES_OK);

  CHECK(SOLPARSER_ID_IS_VALID(sun->spectrum), 0);

  CHECK(solparser_load(parser), RES_BAD_OP);
  fclose(stream);
}

int
main(int argc, char** argv)
{
  struct mem_allocator allocator;
  struct solparser* parser;
  (void)argc, (void)argv;

  CHECK(mem_init_proxy_allocator(&allocator, &mem_default_allocator), RES_OK);
  CHECK(solparser_create(&allocator, &parser), RES_OK);

  test_sun(parser);

  solparser_ref_put(parser);

  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}

