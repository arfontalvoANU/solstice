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
 * Helper functions
 ******************************************************************************/
static res_T
parse_buie
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* buie,
   struct solparser_sun_buie* sun)
{
  enum { CSR };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && buie && sun);

  if(buie->type != YAML_MAPPING_NODE) {
    log_err(parser, buie,
      "expect a buie definition of the sun radial angular distribution.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = buie->data.mapping.pairs.top - buie->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, buie->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, buie->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a buie parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    if(!strcmp((char*)key->data.scalar.value, "csr")) {
      if(mask & BIT(CSR)) {
        log_err(parser, key, "the buie `csr' is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(CSR);
      res = parse_real(parser, val, 1e-6, 0.849, &sun->csr);
    } else {
      log_err(parser, key, "unknown buie parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }

  if(!(mask & BIT(CSR))) {
    log_err(parser, buie, "the buie csr parameter is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_pillbox
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* pillbox,
   struct solparser_sun_pillbox* sun)
{
  enum { THETA_MAX };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && pillbox && sun);

  if(pillbox->type != YAML_MAPPING_NODE) {
    log_err(parser, pillbox,
      "expect a pillbox definition of the sun radial angular distribution.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = pillbox->data.mapping.pairs.top - pillbox->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, pillbox->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, pillbox->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a pillbox parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "theta_max")) {
      if(mask & BIT(THETA_MAX)) {
        log_err(parser, key, "the pillbox `theta_max' is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(THETA_MAX);
      res = parse_real(parser, val, nextafter(0, 1), 90, &sun->theta_max);
    } else {
      log_err(parser, pillbox, "unknown pillbox parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }

  if(!(mask & BIT(THETA_MAX))) {
    log_err(parser, pillbox, "the pillbox theta_max parameter is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
parse_sun
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sun,
   struct solparser_sun** out_solsun)
{
  enum { DNI, RADIAL_ANGULAR_DISTRIB, SPECTRUM };
  struct solparser_sun* solsun = NULL;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sun && out_solsun);

  if(sun == parser->sun_key) {
    solsun = &parser->sun;
    goto exit;
  } else if(parser->sun_key != 0) {
    log_err(parser, sun,
      "a sun is already defined. Previous definition is here %lu:%lu.\n",
      (unsigned long)parser->sun_key->start_mark.line+1,
      (unsigned long)parser->sun_key->start_mark.column+1);
    res = RES_BAD_ARG;
    goto error;
  } else {
    solsun = &parser->sun;
    parser->sun_key = sun;
    solsun->radang_distrib_type = SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL;
  }

  if(sun->type != YAML_MAPPING_NODE) {
    log_err(parser, sun, "expect a sun definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = sun->data.mapping.pairs.top - sun->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, sun->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, sun->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect sun parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the sun "Name" is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "dni")) {
      SETUP_MASK(DNI, "dni");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &solsun->dni);
    } else if(!strcmp((char*)key->data.scalar.value, "buie")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      solsun->radang_distrib_type = SOLPARSER_SUN_RADANG_DISTRIB_BUIE;
      res = parse_buie(parser, doc, val, &solsun->radang_distrib.buie);
    } else if(!strcmp((char*)key->data.scalar.value, "pillbox")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      solsun->radang_distrib_type = SOLPARSER_SUN_RADANG_DISTRIB_PILLBOX;
      res = parse_pillbox(parser, doc, val, &solsun->radang_distrib.pillbox);
    } else if(!strcmp((char*)key->data.scalar.value, "spectrum")) {
      SETUP_MASK(SPECTRUM, "spectrum");
      res = parse_spectrum(parser, doc, val, 0, DBL_MAX, &solsun->spectrum);
    } else {
      log_err(parser, key, "unknown sun parameter `%s'.\n",
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
      log_err(parser, sun, "the sun "Name" is missing.\n");                    \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(DNI, "dni");
  #undef CHECK_PARAM

exit:
  *out_solsun = solsun;
  return res;
error:
  if(solsun) {
    solparser_sun_clear(solsun);
    solsun = NULL;
    parser->sun_key = 0;
  }
  goto exit;
}


