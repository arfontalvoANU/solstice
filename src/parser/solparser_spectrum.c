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

#define _POSIX_C_SOURCE 200112L /* nextafter support */

#include "solparser_c.h"

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static int
cmp_spectrum_data(const void* op0, const void* op1)
{
  const struct solparser_spectrum_data* a = op0;
  const struct solparser_spectrum_data* b = op1;
  ASSERT(a && b);
  if(a->wavelength < b->wavelength) return -1;
  if(a->wavelength > b->wavelength) return 1;
  return 0;
}

static res_T
parse_spectrum_data
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sdata,
   struct solparser_spectrum_data* spectrum_data)
{
  enum { DATA, WAVELENGTH };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sdata && spectrum_data);

  if(sdata->type != YAML_MAPPING_NODE) {
    log_err(parser, sdata, "expect the definition of a spectrum data.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = sdata->data.mapping.pairs.top - sdata->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, sdata->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, sdata->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a spectrum data parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the `"Name"' of the spectrum data is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "data")) {
      SETUP_MASK(DATA, "data");
      res = parse_real(parser, val, 0, DBL_MAX, &spectrum_data->data);
    } else if(!strcmp((char*)key->data.scalar.value, "wavelength")) {
      SETUP_MASK(WAVELENGTH, "wavelength");
      res = parse_real(parser, val, nextafter(0, DBL_MAX), DBL_MAX,
        &spectrum_data->wavelength);
    } else {
      log_err(parser, key, "unknown spectrum data parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, sdata,"the "Name" of the spectrum data is missing.\n");  \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(DATA, "data");
  CHECK_PARAM(WAVELENGTH, "wavelength");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
parse_spectrum
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* spectrum,
   struct solparser_spectrum_id* out_ispectrum)
{
  struct solparser_spectrum* spec;
  size_t ispec = SIZE_MAX;
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && spectrum && out_ispectrum);

  if(spectrum->type != YAML_SEQUENCE_NODE) {
    log_err(parser, spectrum, "expect a list of spectrum data.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the spectrum */
  ispec = darray_spectrum_size_get(&parser->spectra);
  res = darray_spectrum_resize(&parser->spectra, ispec + 1);
  if(res != RES_OK) {
    log_err(parser, spectrum, "could not allocate the spectrum.\n");
    goto error;
  }
  spec = darray_spectrum_data_get(&parser->spectra) + ispec;

  n = spectrum->data.sequence.items.top - spectrum->data.sequence.items.start;
  res = darray_spectrum_data_resize(&spec->data, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, spectrum, "could not allocate the list of spectrum data.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* sdata;
    struct solparser_spectrum_data* spectrum_data;

    sdata = yaml_document_get_node(doc, spectrum->data.sequence.items.start[i]);
    spectrum_data = darray_spectrum_data_data_get(&spec->data) + i;
    res = parse_spectrum_data(parser, doc, sdata, spectrum_data);
    if(res != RES_OK) goto error;
  }

  if(n == 1) goto exit;

  qsort
    (darray_spectrum_data_data_get(&spec->data),
     darray_spectrum_data_size_get(&spec->data),
     sizeof(struct solparser_spectrum_data),
     cmp_spectrum_data);

  FOR_EACH(i, 1, n) {
    const struct solparser_spectrum_data* a;
    const struct solparser_spectrum_data* b;
    a = darray_spectrum_data_cdata_get(&spec->data) + i - 1;
    b = darray_spectrum_data_cdata_get(&spec->data) + i;
    ASSERT(cmp_spectrum_data(a, b) <= 0);
    if(a->wavelength == b->wavelength) {
      log_err(parser, spectrum,
        "duplicated spectrum entry for the wavelength %g\n", a->wavelength);
      res = RES_BAD_ARG;
      goto error;
    }
  }

exit:
  out_ispectrum->i = ispec;
  return res;
error:
  if(spec) {
    darray_spectrum_pop_back(&parser->spectra);
    ispec = SIZE_MAX;
  }
  goto exit;
}

