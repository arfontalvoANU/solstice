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
#include <math.h> /* nextafter */

/*******************************************************************************
* Local function
******************************************************************************/
res_T
parse_atmosphere
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* atm,
   struct solparser_atmosphere** out_solatm)
{
  enum { ABSORPTION };
  struct solparser_atmosphere* solatm = NULL;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && atm && out_solatm);

  if(atm == parser->atmosphere_key) {
    solatm = &parser->atmosphere;
    goto exit;
  } else if(parser->atmosphere_key != 0) {
    log_err(parser, atm,
      "an atmosphere is already defined. Previous definition is here %lu:%lu.\n",
      (unsigned long)parser->atmosphere_key->start_mark.line + 1,
      (unsigned long)parser->atmosphere_key->start_mark.column + 1);
    res = RES_BAD_ARG;
    goto error;
  } else {
    solatm = &parser->atmosphere;
    parser->atmosphere_key = atm;
  }

  if(atm->type != YAML_MAPPING_NODE) {
    log_err(parser, atm, "expect a mapping of atmosphere attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = atm->data.mapping.pairs.top - atm->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, atm->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, atm->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an atmosphere parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                          \
      if(mask & BIT(Flag)) {                                                  \
         log_err(parser, key,                                                 \
           "the "Name" of the atmosphere is already defined.\n");             \
         res = RES_BAD_ARG;                                                   \
         goto error;                                                          \
      }                                                                       \
      mask |= BIT(Flag);                                                      \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "absorption")) {
      SETUP_MASK(ABSORPTION, "absorption");
      res = parse_mtl_data
        (parser, doc, val, 0, 1, &solatm->absorption);
    } else {
      log_err(parser, key, "unknown atmosphere parameter `%s'.\n",
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

  #define CHECK_PARAM(Flag, Name)                                             \
  if(!(mask & BIT(Flag))) {                                                   \
    log_err(parser, atm, "the "Name" of the atmosphere is missing.\n");       \
    res = RES_BAD_ARG;                                                        \
    goto error;                                                               \
  } (void)0
  CHECK_PARAM(ABSORPTION, "absorption");
  #undef CHECK_PARAM
  
exit:
  *out_solatm = solatm;
  return res;
error:
  if(solatm) {
    solparser_atmosphere_clear(solatm);
    solatm = NULL;
    parser->atmosphere_key = 0;
  }
  solatm = NULL;
  goto exit;
}

