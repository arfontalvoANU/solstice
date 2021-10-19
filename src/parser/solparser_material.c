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

#define _POSIX_C_SOURCE 200112L /* nextafter support */

#include "solparser_c.h"
#include <math.h> /* nextafter */

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static res_T
parse_microfacet
  (struct solparser* parser,
   const yaml_node_t* microfacet,
   enum solparser_microfacet_distribution* distrib)
{
  res_T res = RES_OK;
  ASSERT(microfacet && distrib);

  if(microfacet->type != YAML_SCALAR_NODE) {
    log_err(parser, microfacet,
      "expect the name of a microfacet distribution.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)microfacet->data.scalar.value, "BECKMANN")) {
    *distrib = SOLPARSER_MICROFACET_BECKMANN;
  } else if(!strcmp((char*)microfacet->data.scalar.value, "PILLBOX")) {
    *distrib = SOLPARSER_MICROFACET_PILLBOX;
  } else {
    log_err(parser, microfacet, "unknown microfacet distribution `%s'.\n",
      microfacet->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_material_dielectric
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* dielec,
   struct solparser_material_dielectric_id* out_imtl)
{
  enum { MEDIUM_I, MEDIUM_T, NORMAL_MAP };
  struct solparser_material_dielectric* mtl = NULL;
  size_t imtl = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && dielec && out_imtl);

  if(dielec->type != YAML_MAPPING_NODE) {
    log_err(parser, dielec,
      "expect a mapping of dielec material attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the dielec material */
  imtl = darray_dielectric_size_get(&parser->dielectrics);
  res = darray_dielectric_resize(&parser->dielectrics, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, dielec,
      "could not allocate the dielec material.\n");
    goto error;
  }
  mtl = darray_dielectric_data_get(&parser->dielectrics) + imtl;

  n = dielec->data.mapping.pairs.top - dielec->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, dielec->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, dielec->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a dielec material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" of the dielectric material is already defined.\n");      \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "normal_map")) {
      SETUP_MASK(NORMAL_MAP, "normal_map");
      res = parse_image(parser, doc, val, &mtl->normal_map);
    } else if(!strcmp((char*)key->data.scalar.value, "medium_i")) {
      SETUP_MASK(MEDIUM_I, "medium_i");
      res = parse_medium(parser, doc, val, &mtl->medium_i);
    } else if(!strcmp((char*)key->data.scalar.value, "medium_t")) {
      SETUP_MASK(MEDIUM_T, "medium_t");
      res = parse_medium(parser, doc, val, &mtl->medium_t);
    } else {
      log_err(parser, key, "unknown dielectric parameter `%s'.\n",
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
      log_err(parser, dielec,                                                  \
        "the "Name" of the dielectric material is missing.\n");                \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(MEDIUM_I, "medium_i");
  CHECK_PARAM(MEDIUM_T, "medium_t");
  #undef CHECK_PARAM

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(imtl) {
    darray_dielectric_pop_back(&parser->dielectrics);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material_matte
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* matte,
   struct solparser_material_matte_id* out_imtl)
{
  enum { NORMAL_MAP, REFLECTIVITY };
  struct solparser_material_matte* mtl = NULL;
  size_t imtl = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && matte && out_imtl);

  if(matte->type != YAML_MAPPING_NODE) {
    log_err(parser, matte, "expect a mapping of matte material parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the matte material */
  imtl = darray_matte_size_get(&parser->mattes);
  res = darray_matte_resize(&parser->mattes, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, matte, "could not allocate the matte material.\n");
    goto error;
  }
  mtl = darray_matte_data_get(&parser->mattes) + imtl;

  n = matte->data.mapping.pairs.top - matte->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, matte->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, matte->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a matte material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" of the matte material is already defined.\n");           \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "normal_map")) {
      SETUP_MASK(NORMAL_MAP, "normal_map");
      res = parse_image(parser, doc, val, &mtl->normal_map);
    } else if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      SETUP_MASK(REFLECTIVITY, "reflectivity");
      res = parse_mtl_data(parser, doc, val, 0, 1, &mtl->reflectivity);
    } else {
      log_err(parser, key, "unknown matte parameter `%s'.\n",
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

  if(!(mask & BIT(REFLECTIVITY))) {
    log_err(parser, matte, "the matte reflectivity is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_matte_pop_back(&parser->mattes);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material_mirror
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* mirror,
   struct solparser_material_mirror_id* out_imtl)
{
  enum { MICROFACET, NORMAL_MAP, REFLECTIVITY, SLOPE_ERROR };
  struct solparser_material_mirror* mtl = NULL;
  size_t imtl = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && mirror && out_imtl);

  if(mirror->type != YAML_MAPPING_NODE) {
    log_err(parser, mirror,
      "expect a mapping of mirror material attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the mirror material */
  imtl = darray_mirror_size_get(&parser->mirrors);
  res = darray_mirror_resize(&parser->mirrors, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, mirror, "could not allocate the mirror material.\n");
    goto error;
  }
  mtl = darray_mirror_data_get(&parser->mirrors) + imtl;

  n = mirror->data.mapping.pairs.top - mirror->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, mirror->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, mirror->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a mirror material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" of the mirror material is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "microfacet")) {
      SETUP_MASK(MICROFACET, "microfacet");
      res = parse_microfacet(parser, val, &mtl->ufacet_distrib);
    } else if(!strcmp((char*)key->data.scalar.value, "normal_map")) {
      SETUP_MASK(NORMAL_MAP, "normal_map");
      res = parse_image(parser, doc, val, &mtl->normal_map);
    } else if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      SETUP_MASK(REFLECTIVITY, "reflectivity");
      res = parse_mtl_data(parser, doc, val, 0, 1, &mtl->reflectivity);
    } else if(!strcmp((char*)key->data.scalar.value, "slope_error")) {
      SETUP_MASK(SLOPE_ERROR, "slope_error");
      res = parse_mtl_data(parser, doc, val, 0, 1, &mtl->slope_error);
    } else {
      log_err(parser, key, "unknown mirror attribute `%s'.\n",
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
      log_err(parser, mirror, "the mirror "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(REFLECTIVITY, "reflectivity");
  CHECK_PARAM(SLOPE_ERROR, "slope_error");
  #undef CHECK_PARAM

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_mirror_pop_back(&parser->mirrors);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material_thin_dielectric
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* thin,
   struct solparser_material_thin_dielectric_id* out_imtl)
{
  enum { MEDIUM_I, MEDIUM_T, NORMAL_MAP, THICKNESS };
  struct solparser_material_thin_dielectric* mtl = NULL;
  size_t imtl = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && thin && out_imtl);

  if(thin->type != YAML_MAPPING_NODE) {
    log_err(parser, thin,
      "expect a mapping of thin dielectric material attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the thin dielectric material */
  imtl = darray_thin_dielectric_size_get(&parser->thin_dielectrics);
  res = darray_thin_dielectric_resize(&parser->thin_dielectrics, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, thin,
      "could not allocate the thin dielectric material.\n");
    goto error;
  }
  mtl = darray_thin_dielectric_data_get(&parser->thin_dielectrics) + imtl;

  n = thin->data.mapping.pairs.top - thin->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, thin->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, thin->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a thin dielectric material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" of the thin dielectric material is already defined.\n"); \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "normal_map")) {
      SETUP_MASK(NORMAL_MAP, "normal_map");
      res = parse_image(parser, doc, val, &mtl->normal_map);
    } else if(!strcmp((char*)key->data.scalar.value, "medium_i")) {
      SETUP_MASK(MEDIUM_I, "medium_i");
      res = parse_medium(parser, doc, val, &mtl->medium_i);
    } else if(!strcmp((char*)key->data.scalar.value, "medium_t")) {
      SETUP_MASK(MEDIUM_T, "medium_t");
      res = parse_medium(parser, doc, val, &mtl->medium_t);
    } else if(!strcmp((char*)key->data.scalar.value, "thickness")) {
      SETUP_MASK(THICKNESS, "thickness");
      res = parse_real(parser, val, 0, DBL_MAX, &mtl->thickness);
    } else {
      log_err(parser, key, "unknown thin dielectric parameter `%s'.\n",
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
      log_err(parser, thin,                                                    \
        "the "Name" of the thin dielectric material is missing.\n");           \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(MEDIUM_I, "medium_i");
  CHECK_PARAM(MEDIUM_T, "medium_t");
  CHECK_PARAM(THICKNESS, "thickness");
  #undef CHECK_PARAM

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_thin_dielectric_pop_back(&parser->thin_dielectrics);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material_virtual(struct solparser* parser, yaml_node_t* virtual)
{
  res_T res = RES_OK;
  ASSERT(virtual);

  if(virtual->type != YAML_SCALAR_NODE
  || ((char*)virtual->data.scalar.value)[0] != '\0') {
    log_err(parser, virtual,
      "virtual materials can have a null scalar value only.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_material_descriptor
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* desc,
   struct solparser_material_id* out_imtl)
{
  enum { DESCRIPTOR };
  struct solparser_material* mtl = NULL;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  size_t* pimtl;
  size_t imtl = SIZE_MAX;
  res_T res = RES_OK;
  ASSERT(doc && desc && out_imtl);

  /* Check whether or not the YAML descriptor alias an already created Solstice
   * material */
  pimtl = htable_yaml2sols_find(&parser->yaml2mtls, &desc);
  if(pimtl) {
    imtl = *pimtl;
    goto exit;
  }

  /* Allocate the solstice material */
  imtl = darray_material_size_get(&parser->mtls);
  res = darray_material_resize(&parser->mtls, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, desc, "could not allocate the material descriptor.\n");
    goto error;
  }
  mtl = darray_material_data_get(&parser->mtls) + imtl;

  if(desc->type != YAML_MAPPING_NODE) {
    log_err(parser, desc, "expect a material descriptor.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = desc->data.mapping.pairs.top - desc->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, desc->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, desc->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a material name.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the material "Name" is already defined.\n");     \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "dielectric")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_DIELECTRIC;
      res = parse_material_dielectric(parser, doc, val, &mtl->data.dielectric);
    } else if(!strcmp((char*)key->data.scalar.value, "matte")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_MATTE;
      res = parse_material_matte(parser, doc, val, &mtl->data.matte);
    } else if(!strcmp((char*)key->data.scalar.value, "mirror")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_MIRROR;
      res = parse_material_mirror(parser, doc, val, &mtl->data.mirror);
    } else if(!strcmp((char*)key->data.scalar.value, "thin_dielectric")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_THIN_DIELECTRIC;
      res = parse_material_thin_dielectric
        (parser, doc, val, &mtl->data.thin_dielectric);
    } else if(!strcmp((char*)key->data.scalar.value, "virtual")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_VIRTUAL;
      res = parse_material_virtual(parser, val);
    } else {
      log_err(parser, key, "unknown material descriptor `%s'.\n",
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

  if(!(mask & BIT(DESCRIPTOR))) {
    log_err(parser, desc, "the material descriptor is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Cache the material */
  res = htable_yaml2sols_set(&parser->yaml2mtls, &desc, &imtl);
  if(res != RES_OK) {
    log_err(parser, desc, "could not register the material.\n");
    goto error;
  }

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_material_pop_back(&parser->mtls);
    imtl = SIZE_MAX;
  }
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
parse_material
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* mtl,
   struct solparser_material_double_sided_id* out_imtl2)
{
  enum { FRONT, BACK };
  struct solparser_material_double_sided* mtl2 = NULL;
  size_t imtl2 = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && mtl && out_imtl2);

  if(mtl->type != YAML_MAPPING_NODE) {
    log_err(parser, mtl, "expect a material definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the double sided material */
  imtl2 = darray_material2_size_get(&parser->mtls2);
  res = darray_material2_resize(&parser->mtls2, imtl2 + 1);
  if(res != RES_OK) {
    log_err(parser, mtl, "could not allocate the material.\n");
    goto error;
  }
  mtl2 = darray_material2_data_get(&parser->mtls2) + imtl2;

  n = mtl->data.mapping.pairs.top - mtl->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, mtl->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, mtl->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key,
        "expect a material descriptor or a double sided material.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" material descriptor is already defined.\n");             \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "front")) {
      SETUP_MASK(FRONT, "front");
      res = parse_material_descriptor(parser, doc, val, &mtl2->front);
    } else if(!strcmp((char*)key->data.scalar.value, "back")) {
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(parser, doc, val, &mtl2->back);
    } else {
      SETUP_MASK(FRONT, "front");
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(parser, doc, mtl, &mtl2->front);
      mtl2->back = mtl2->front;
      if(res != RES_OK) goto error; /* Discard log_node */
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                             \
    if(!(mask & BIT(Flag))) {                                                 \
      log_err(parser, mtl, "the "Name" material descriptor is missing.\n");   \
      res = RES_BAD_ARG;                                                      \
      goto error;                                                             \
    } (void)0
  CHECK_PARAM(FRONT, "front");
  CHECK_PARAM(BACK, "back");
  #undef CHECK_PARAM

exit:
  out_imtl2->i = imtl2;
  return res;
error:
  if(mtl2) {
    darray_material2_pop_back(&parser->mtls2);
    imtl2 = SIZE_MAX;
  }
  goto exit;
}

