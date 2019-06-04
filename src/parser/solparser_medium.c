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

#define _POSIX_C_SOURCE 200112L /* nextafter support */

#include "solparser_c.h"
#include <math.h> /* nextafter */

/*******************************************************************************
 * Local function
 ******************************************************************************/
res_T
parse_medium
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* medium,
   struct solparser_medium_id* out_imedium)
{
  enum { EXTINCTION, REFRACTIVE_INDEX };
  struct solparser_medium* mdm = NULL;
  size_t* pimedium = NULL;
  size_t imedium = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && medium && out_imedium);

  if(medium->type != YAML_MAPPING_NODE) {
    log_err(parser, medium, "expect a mapping of medium attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Check whether or not the YAML medium alias an already created Solstice
   * medium */
  pimedium = htable_yaml2sols_find(&parser->yaml2mediums, &medium);
  if(pimedium) {
    imedium = *pimedium;
    goto exit;
  }

  /* Allocate the medium */
  imedium = darray_medium_size_get(&parser->mediums);
  res = darray_medium_resize(&parser->mediums, imedium + 1);
  if(res != RES_OK) {
    log_err(parser, medium, "could not allocate the medium.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  mdm = darray_medium_data_get(&parser->mediums) + imedium;

  n = medium->data.mapping.pairs.top - medium->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, medium->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, medium->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a medium parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
         log_err(parser, key,"the "Name" of the medium is already defined.\n");\
         res = RES_BAD_ARG;                                                    \
         goto error;                                                           \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "extinction")) {
      SETUP_MASK(EXTINCTION, "extinction");
      res = parse_mtl_data(parser, doc, val, 0, DBL_MAX, &mdm->extinction);
    } else if(!strcmp((char*)key->data.scalar.value, "refractive_index")) {
      SETUP_MASK(REFRACTIVE_INDEX, "refractive_index");
      res = parse_mtl_data
        (parser, doc, val, nextafter(0, 1), DBL_MAX, &mdm->refractive_index);
    } else {
      log_err(parser, key, "unknown medium parameter `%s'.\n",
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
  if(!(mask & BIT(Flag))) {                                                    \
    log_err(parser, medium, "the "Name" of the medium is missing.\n");         \
    res = RES_BAD_ARG;                                                         \
    goto error;                                                                \
  } (void)0
  CHECK_PARAM(EXTINCTION, "absorption");
  CHECK_PARAM(REFRACTIVE_INDEX, "refractive_index");
  #undef CHECK_PARAM

  /* Cache the medium */
  res = htable_yaml2sols_set(&parser->yaml2mediums, &medium, &imedium);
  if(res != RES_OK) {
    log_err(parser, medium, "could not register the medium.\n");
    goto error;
  }

exit:
  out_imedium->i = imedium;
  return res;
error:
  if(imedium) {
    darray_medium_pop_back(&parser->mediums);
    imedium = SIZE_MAX;
  }
  goto exit;
}

