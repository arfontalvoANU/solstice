/* Copyright (C) CNRS 2016
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

#include "solstice_parser.h"

#include <rsys/cstr.h>
#include <rsys/double3.h>
#include <rsys/ref_count.h>
#include <rsys/str.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

struct solstice_parser {
  yaml_parser_t parser;
  struct str stream_name;
  int parser_is_init;

  ref_T ref;
  struct mem_allocator* allocator;
};

enum paraboloid_type {
  PARABOL,
  PARABOLIC_CYLINDER
};

enum geometry_fileformat {
  GEOMETRY_OBJ,
  GEOMETRY_STL
};

static res_T
parse_node
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* children);

static res_T
parse_object
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* object);

static res_T
parse_pivot
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* pivot);

static res_T
parse_sun
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sun);

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static INLINE void
log_err
  (const struct solstice_parser* parser,
   const yaml_node_t* node,
   const char* fmt,
   ...)
{
  va_list vargs_list;
  ASSERT(parser && node && fmt);

  fprintf(stderr, "%s:%lu:%lu: ",
    str_cget(&parser->stream_name),
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
  va_start(vargs_list, fmt);
  vfprintf(stderr, fmt, vargs_list);
  va_end(vargs_list);
}

static INLINE const char*
paraboloid_type_name(const enum paraboloid_type type)
{
  switch(type) {
    case PARABOL: return "parabol"; break;
    case PARABOLIC_CYLINDER: return "parabolic cylinder"; break;
    default: FATAL("Unreachable code.\n"); break;
  }
}

static INLINE const char*
geometry_fileformat_name(const enum geometry_fileformat fileformat)
{
  switch(fileformat) {
    case GEOMETRY_OBJ: return "obj"; break;
    case GEOMETRY_STL: return "stl"; break;
    default: FATAL("Unreachable code.\n"); break;
  }
}

static void
parser_release(ref_T* ref)
{
  struct solstice_parser* parser;
  ASSERT(ref);

  parser = CONTAINER_OF(ref, struct solstice_parser, ref);
  if(parser->parser_is_init) yaml_parser_delete(&parser->parser);
  str_release(&parser->stream_name);
  MEM_RM(parser->allocator, parser);
}

/*******************************************************************************
 * Miscellaneous parsing functions
 ******************************************************************************/
static res_T
parse_real
  (struct solstice_parser* parser,
   const yaml_node_t* real,
   const double lower_bound,
   const double upper_bound,
   double* dst)
{
  res_T res = RES_OK;
  ASSERT(real && dst && lower_bound < upper_bound);

  if(real->type != YAML_SCALAR_NODE
  || !strlen((char*)real->data.scalar.value)) {
    log_err(parser, real, "expect a floating point number.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  res = cstr_to_double((char*)real->data.scalar.value, dst);
  if(res != RES_OK) {
    log_err(parser, real, "invalid floating point number `%s'.\n",
      real->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

  if(*dst < lower_bound || *dst > upper_bound) {
    log_err(parser, real, "%g is not in [%g, %g].\n",
      *dst, lower_bound, upper_bound);
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_real3
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* real3,
   const double lower_bound,
   const double upper_bound,
   double dst[3])
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && real3 && dst);

  if(real3->type != YAML_SEQUENCE_NODE) {
    log_err(parser, real3, "expect a sequence of 3 reals.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = real3->data.sequence.items.top - real3->data.sequence.items.start;
  if(n != 3) {
    log_err(parser, real3, "expect 3 reals while `%li' %s submitted.\n",
      n, n > 1 ? "are" : "is");
    res = RES_BAD_ARG;
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* real;
    real = yaml_document_get_node(doc, real3->data.sequence.items.start[i]);
    res = parse_real(parser, real, lower_bound, upper_bound, dst + i);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_integer
  (struct solstice_parser* parser,
   yaml_node_t* integer,
   const long lower_bound,
   const long upper_bound,
   long* dst)
{
  res_T res = RES_OK;
  ASSERT(integer && dst && lower_bound < upper_bound);

  if(integer->type != YAML_SCALAR_NODE
  || !strlen((char*)integer->data.scalar.value)) {
    log_err(parser, integer, "expect an integer.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  res = cstr_to_long((char*)integer->data.scalar.value, dst);
  if(res != RES_OK) {
    log_err(parser, integer, "invalid integer `%s'.\n",
      integer->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

  if(*dst < lower_bound || *dst > upper_bound) {
    log_err(parser, integer, "%li is not in [%li, %li].\n",
      *dst, lower_bound, upper_bound);
    res = RES_BAD_ARG;
    goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_string(struct solstice_parser* parser, yaml_node_t* string)
{
  res_T res = RES_OK;
  ASSERT(string);

  if(string->type != YAML_SCALAR_NODE
  || !strlen((char*)string->data.scalar.value)) {
    log_err(parser, string, "expect a character string.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  /* TODO create a string from the value of the YAML scalar node */
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_transform
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* transform,
   double translation[3],
   double rotation[3])
{
  enum { ROTATION, TRANSLATION };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && translation && rotation && transform);

  d3_splat(translation, 0);
  d3_splat(rotation, 0);

  if(transform->type != YAML_MAPPING_NODE) {
    log_err(parser, transform, "expect a mapping of transform parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = transform->data.mapping.pairs.top - transform->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, transform->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, transform->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect transform parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the transform `"Name"' is already defined.\n");  \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "translation")) {
      SETUP_MASK(TRANSLATION, "translation");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, translation);
    } else if(!strcmp((char*)key->data.scalar.value, "rotation")) {
      SETUP_MASK(ROTATION, "rotation");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, rotation);
    } else {
      log_err(parser, key, "unknown transform parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_spectrum_data
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const double lower_bound,
   const double upper_bound,
   const yaml_node_t* sdata)
{
  enum { DATA, WAVELENGTH };
  double wavelength;
  double data;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && data && lower_bound < upper_bound);

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
      res = parse_real(parser, val, lower_bound, upper_bound, &data);
    } else if(!strcmp((char*)key->data.scalar.value, "wavelength")) {
      SETUP_MASK(WAVELENGTH, "wavelength");
      res = parse_real(parser, val, 0, DBL_MAX, &wavelength);
    } else {
      log_err(parser, key, "unknown spectrum data parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
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

static res_T
parse_spectrum
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const double lower_bound,
   const double upper_bound,
   const yaml_node_t* spectrum)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && spectrum && lower_bound < upper_bound);

  if(spectrum->type != YAML_SEQUENCE_NODE) {
    log_err(parser, spectrum, "expect a list of spectrum data.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = spectrum->data.sequence.items.top - spectrum->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* sdata;

    sdata = yaml_document_get_node(doc, spectrum->data.sequence.items.start[i]);
    res = parse_spectrum_data(parser, doc, lower_bound, upper_bound, sdata);
    if(res != RES_OK) goto error;

    /* TODO register the spectrum data */
  }

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Instance
 ******************************************************************************/
static res_T
parse_instance
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* inst)
{
  enum { GEOMETRY, TRANSFORM };
  double translation[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && inst);

  if(inst->type != YAML_MAPPING_NODE) {
    log_err(parser, inst, "expect an instance definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = inst->data.mapping.pairs.top - inst->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, inst->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, inst->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect instance parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the instance "Name" is already defined.\n");     \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "node")) {
      SETUP_MASK(GEOMETRY, "geometry");
      res = parse_node(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "object")) {
      SETUP_MASK(GEOMETRY, "geometry");
      res = parse_object(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(parser, doc, val, translation, rotation);
    } else {
      log_err(parser, key, "unknown instance parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, inst, "the instance "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(GEOMETRY, "geometry");
  #undef CHECK_PARAM

  /* TODO register the instance */

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Material
 ******************************************************************************/
static res_T
parse_material_matte
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* matte)
{
  enum { REFLECTIVITY };
  double reflectivity;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && matte);

  if(matte->type != YAML_MAPPING_NODE) {
    log_err(parser, matte, "expect a mapping of matte material parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

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

    if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      if(mask & BIT(REFLECTIVITY)) {
        log_err(parser, key, "the matte reflectivity is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(REFLECTIVITY);
      res = parse_real(parser, val, 0, 1, &reflectivity);
    } else {
      log_err(parser, key, "unknown matte parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

  if(!(mask & BIT(REFLECTIVITY))) {
    log_err(parser, matte, "the matte reflectivity is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* TODO create the matte material */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_material_mirror
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* mirror)
{
  enum { REFLECTIVITY, ROUGHNESS };
  double reflectivity;
  double roughness;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && mirror);

  if(mirror->type != YAML_MAPPING_NODE) {
    log_err(parser, mirror,
      "expect a mapping of mirror material attributes .\n");
    res = RES_BAD_ARG;
    goto error;
  }
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
    if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      SETUP_MASK(REFLECTIVITY, "reflectivity");
      res = parse_real(parser, val, 0, 1, &reflectivity);
    } else if(!strcmp((char*)key->data.scalar.value, "roughness")) {
      SETUP_MASK(ROUGHNESS, "roughness");
      res = parse_real(parser, val, 0, 1, &roughness);
    } else {
      log_err(parser, key, "unknown mirror attribute `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, mirror, "the mirror "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(REFLECTIVITY, "reflectivity");
  CHECK_PARAM(ROUGHNESS, "roughness");
  #undef CHECK_PARAM

  /* TODO create the mirror material */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_material_descriptor
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* desc)
{
  enum { DESCRIPTOR };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && desc);

  /* TODO If the descriptor is an alias of an already created material skip the
   * parsing and return the aliased material descriptor */

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
    if(!strcmp((char*)key->data.scalar.value, "matte")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      res = parse_material_matte(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "mirror")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      res = parse_material_mirror(parser, doc, val);
    } else {
      log_err(parser, key, "unknown material descriptor `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  if(!(mask & BIT(DESCRIPTOR))) {
    log_err(parser, desc, "the material descriptor is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_material
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* mtl)
{
  enum { FRONT, BACK };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && mtl);

  if(mtl->type != YAML_MAPPING_NODE) {
    log_err(parser, mtl, "expect a material definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

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
      res = parse_material_descriptor(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "back")) {
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(parser, doc, val);
    } else {
      SETUP_MASK(FRONT, "front");
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(parser, doc, mtl);
    }
    if(res != RES_OK) goto error;
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
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Clipping polygon
 ******************************************************************************/
static res_T
parse_clip_op(struct solstice_parser* parser, const yaml_node_t* op)
{
  res_T res = RES_OK;
  ASSERT(op);

  if(op->type != YAML_SCALAR_NODE) {
    log_err(parser, op, "expect a clipping operation.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)op->data.scalar.value, "AND")) { /* TODO setup op */
  } else if(!strcmp((char*)op->data.scalar.value, "SUB")) { /* TODO setup op */
  } else {
    log_err(parser, op, "unknown clipping operation `%s'.\n",
      op->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_vertices
  (struct solstice_parser* parser, yaml_document_t* doc, const yaml_node_t* vertices)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && vertices);

  if(vertices->type != YAML_SEQUENCE_NODE) {
    log_err(parser, vertices, "expect a list of vertices.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = vertices->data.sequence.items.top - vertices->data.sequence.items.start;
  if(n < 3) {
    log_err(parser, vertices, "expect at least 3 vertices.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* vertex;
    double coords[3];

    vertex = yaml_document_get_node(doc, vertices->data.sequence.items.start[i]);
    res = parse_real3(parser, doc, vertex, -DBL_MAX, DBL_MAX, coords);
    if(res != RES_OK) goto error;

    /* TODO register the vertex */
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_polyclip
  (struct solstice_parser* parser, yaml_document_t* doc, const yaml_node_t* polyclip)
{
  enum { OPERATION, VERTICES };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && polyclip);

  if(polyclip->type != YAML_MAPPING_NODE) {
    log_err(parser, polyclip,
      "expect a mapping of clipping polygon parameters.\n");
    res = RES_OK;
    goto error;
  }

  n = polyclip->data.mapping.pairs.top - polyclip->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, polyclip->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, polyclip->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a clipping polygon parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the clipping polygon parameter `"Name"' is already defined.\n");    \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "operation")) {
      SETUP_MASK(OPERATION, "operation");
      res = parse_clip_op(parser, val);
    } else if(!strcmp((char*)key->data.scalar.value, "vertices")) {
      SETUP_MASK(VERTICES, "vertices");
      res = parse_vertices(parser, doc, val);
    } else {
      log_err(parser, key, "unknown clipping polygon parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, polyclip,                                                \
        "the clipping polygon parameter `"Name"' is missing");                 \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(OPERATION, "operation");
  CHECK_PARAM(VERTICES, "vertices");
  #undef CHECK_PARAM

  /* TODO register the polyclip */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_clip
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* clip)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && clip);

  if(clip->type != YAML_SEQUENCE_NODE) {
    log_err(parser, clip, "expect a list of clipping polygons.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = clip->data.sequence.items.top - clip->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* polyclip;

    polyclip = yaml_document_get_node(doc, clip->data.sequence.items.start[i]);
    res = parse_polyclip(parser, doc, polyclip);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Shapes
 ******************************************************************************/
static res_T
parse_cuboid
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* cuboid)
{
  enum { SIZE };
  double size[3];
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && cuboid);

  if(cuboid->type != YAML_MAPPING_NODE) {
    log_err(parser, cuboid, "expect a mapping of cuboid parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = cuboid->data.mapping.pairs.top - cuboid->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, cuboid->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, cuboid->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect cuboid parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "size")) {
      if(mask & BIT(SIZE)) {
        log_err(parser, key, "the cuboid size is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(SIZE);
      res = parse_real3(parser, doc, val, 0, DBL_MAX, size);
    } else {
      log_err(parser, key, "unknown cuboid parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

  if(!(mask & BIT(SIZE))) {
    log_err(parser, cuboid, "the size of the cuboid is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* TODO register the cuboid */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_cylinder
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* cylinder)
{
  enum { HEIGHT, RADIUS, SLICES };
  double radius;
  double height;
  long nslices = 16;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && cylinder);

  if(cylinder->type != YAML_MAPPING_NODE) {
    log_err(parser, cylinder, "expect a mapping of cylinder parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = cylinder->data.mapping.pairs.top - cylinder->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, cylinder->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, cylinder->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect cylinder parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the cylinder parameter `"Name"' is already defined.\n");            \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "height")) {
      SETUP_MASK(HEIGHT, "height");
      res = parse_real(parser, val, 0, DBL_MAX, &height);
    } else if(!strcmp((char*)key->data.scalar.value, "radius")) {
      SETUP_MASK(RADIUS, "radius");
      res = parse_real(parser, val, 0, DBL_MAX, &radius);
    } else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 4, 4096, &nslices);
    } else {
      log_err(parser, key, "unknown cylinder parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, cylinder,                                                \
        "the cylinder parameter `"Name"' is missing.\n");                      \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(HEIGHT, "height");
  CHECK_PARAM(RADIUS, "radius");
  #undef CHECK_PARAM

  /* TODO register the cylinder */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_imported_geometry
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* geom,
   const enum geometry_fileformat fileformat)
{
  enum { PATH };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  const char* name = geometry_fileformat_name(fileformat);
  res_T res = RES_OK;
  ASSERT(doc && geom);

  if(geom->type != YAML_MAPPING_NODE) {
    log_err(parser, geom, "expect a mapping of %s parameters.\n", name);
    res = RES_BAD_ARG;
    goto error;
  }

  n = geom->data.mapping.pairs.top - geom->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, geom->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, geom->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect %s parameters.\n", name);
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "path")) {
      if(mask & BIT(PATH)) {
        log_err(parser, key, "the %s path is already defined.\n", name);
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(PATH);
      res = parse_string(parser, val);
    } else {
      log_err(parser, key, "unknown %s parameter `%s'.\n",
        name, key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

  if(!(mask & BIT(PATH))) {
    log_err(parser, geom, "the path of the %s geometry is missing.\n", name);
    res = RES_BAD_ARG;
    goto error;
  }

  /* TODO Register the imported geometry */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_paraboloid
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* paraboloid,
   const enum paraboloid_type type)
{
  enum { CLIP, FOCAL };
  double focal;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  const char* name = paraboloid_type_name(type);
  res_T res = RES_OK;
  ASSERT(doc && paraboloid);

  if(paraboloid->type != YAML_MAPPING_NODE) {
    log_err(parser, paraboloid, "expect a mapping of %s parameters.\n", name);
    res = RES_BAD_ARG;
    goto error;
  }

  n = paraboloid->data.mapping.pairs.top - paraboloid->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, paraboloid->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, paraboloid->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect %s parameters.\n", name);
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the %s parameter `"Name"' is already defined.\n", name);            \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "clip")) {
      SETUP_MASK(CLIP, "clip");
      res = parse_clip(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "focal")) {
      SETUP_MASK(FOCAL, "focal");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &focal);
    } else {
      log_err(parser, key, "unknown %s parameter `%s'.\n",
        name, key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }
  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, paraboloid,                                              \
        "the %s parameter `"Name"' is missing.\n", name);                      \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(CLIP, "clip");
  CHECK_PARAM(FOCAL, "focal");
  #undef CHECK_PARAM

  /* TODO register the paraboloid */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_plane
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* plane)
{
  enum { CLIP };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && plane);

  if(plane->type != YAML_MAPPING_NODE) {
    log_err(parser, plane, "expect a mapping of plane parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = plane->data.mapping.pairs.top - plane->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, plane->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, plane->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect plane parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "clip")) {
      if(mask & BIT(CLIP)) {
        log_err(parser, key, "the plane clipping is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(CLIP);
      res = parse_clip(parser, doc, val);
    } else {
      log_err(parser, key, "unknown plane parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }
  if(!(mask & BIT(CLIP))) {
    log_err(parser, plane, "the plane parameter `clip' is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* TODO register the plane */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_sphere
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sphere)
{
  enum { RADIUS, SLICES };
  double radius;
  long nslices;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sphere);

  if(sphere->type != YAML_MAPPING_NODE) {
    log_err(parser, sphere, "expect a mapping of sphere parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = sphere->data.mapping.pairs.top - sphere->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, sphere->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, sphere->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect sphere parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the sphere parameter `"Name"' is already defined.\n");              \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "radius")) {
      SETUP_MASK(RADIUS, "radius");
      res = parse_real(parser, val, 0, DBL_MAX, &radius);
    } else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 4, 4096, &nslices);
    } else {
      log_err(parser, key, "unknown sphere parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  if(!(mask & BIT(RADIUS))) {
    log_err(parser, sphere, "the sphere radius is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  /* TODO register the sphere */

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Object
 ******************************************************************************/
res_T
parse_object
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* object)
{
  enum { MATERIAL, SHAPE, TRANSFORM };
  double translation[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && object);

  /* TODO If the object is an alias of an already created object skip the
   * parsing and return the aliased object */

  if(object->type != YAML_MAPPING_NODE) {
    log_err(parser, object, "expect an object definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = object->data.mapping.pairs.top - object->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, object->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, object->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an object parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the object "Name" is already defined.\n");                          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "material")) {
      SETUP_MASK(MATERIAL, "material");
      res = parse_material(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "cuboid")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_cuboid(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "cylinder")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_cylinder(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "obj")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_imported_geometry(parser, doc, val, GEOMETRY_OBJ);
    } else if(!strcmp((char*)key->data.scalar.value, "parabol")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_paraboloid(parser, doc, val, PARABOL);
    } else if(!strcmp((char*)key->data.scalar.value, "parabolic-cylinder")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_paraboloid(parser, doc, val, PARABOLIC_CYLINDER);
    } else if(!strcmp((char*)key->data.scalar.value, "plane")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_plane(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "sphere")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_sphere(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "stl")) {
      SETUP_MASK(SHAPE, "shape");
      res = parse_imported_geometry(parser, doc, val, GEOMETRY_STL);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(parser, doc, val, translation, rotation);
    } else {
      log_err(parser, key, "unknown object parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, object, "the object "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(MATERIAL, "material");
  CHECK_PARAM(SHAPE, "shape");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Node
 ******************************************************************************/
static res_T
parse_children
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* children)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && children);

  if(children->type != YAML_SEQUENCE_NODE) {
    log_err(parser, children, "expect a list of nodes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = children->data.sequence.items.top - children->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* child;
    yaml_node_t* key;
    yaml_node_t* val;
    intptr_t nb;

    child = yaml_document_get_node(doc, children->data.sequence.items.start[i]);
    if(child->type != YAML_MAPPING_NODE) {
      log_err(parser, child, "expect a node definition.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    nb = child->data.mapping.pairs.top - child->data.mapping.pairs.start;
    if(nb != 1) {
      log_err(parser, child,
        "expect only one \"key:value\" pair while %li are provided.\n", nb);
      res = RES_BAD_ARG;
      goto error;
    }

    key = yaml_document_get_node(doc, child->data.mapping.pairs.start[0].key);
    val = yaml_document_get_node(doc, child->data.mapping.pairs.start[0].value);
    if(!strcmp((char*)key->data.scalar.value, "node")) {
      res = parse_node(parser, doc, val);
    } else {
      log_err(parser, key, "unexpected directive `%s'. Expect a node.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;

    /* TODO register the child node */
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_entities
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* entities)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && entities);

  if(entities->type != YAML_SEQUENCE_NODE) {
    log_err(parser, entities, "expect a list of entities.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = entities->data.sequence.items.top - entities->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* entity;
    yaml_node_t* key;
    yaml_node_t* val;
    intptr_t nb;

    entity = yaml_document_get_node(doc, entities->data.sequence.items.start[i]);

    if(entity->type != YAML_MAPPING_NODE) {
      log_err(parser, entity, "expect an entity definition.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    nb = entity->data.mapping.pairs.top - entity->data.mapping.pairs.start;
    if(nb != 1) {
      log_err(parser, entity,
        "expect only one \"key:value\" pair while %li are provided.\n", nb);
      res = RES_BAD_ARG;
      goto error;
    }

    key = yaml_document_get_node(doc, entity->data.mapping.pairs.start[0].key);
    val = yaml_document_get_node(doc, entity->data.mapping.pairs.start[0].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an entity name.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    if(!strcmp((char*)key->data.scalar.value, "object")) {
      res = parse_object(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "pivot")) {
      res = parse_pivot(parser, doc, val);
    } else {
      log_err(parser, key, "unknown entity `%s'.\n", key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;

    /* TODO register the entity */
  }

exit:
  return res;
error:
  goto exit;
}

res_T
parse_node
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* node)
{
  enum { CHILDREN, ENTITIES, TRANSFORM };
  double translation[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && node);

  if(node->type != YAML_MAPPING_NODE) {
    log_err(parser, node, "expect a node definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = node->data.mapping.pairs.top - node->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, node->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, node->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a node attribute.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the node attribute `"Name"' is already defined.\n");                \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "children")) {
      SETUP_MASK(CHILDREN, "children");
      res = parse_children(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "entities")) {
      SETUP_MASK(ENTITIES, "entities");
      res = parse_entities(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(parser, doc, val, translation, rotation);
    } else {
      log_err(parser, key, "unknown node parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, node, "the node `"Name"' parameter is missing.\n");      \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(ENTITIES, "entities");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Pivot
 ******************************************************************************/
static res_T
parse_target
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* target)
{
  enum { POLICY };
  double direction[3];
  double position[3];
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && target);

  if(target->type != YAML_MAPPING_NODE) {
    log_err(parser, target, "expect a target definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = target->data.mapping.pairs.top - target->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, target->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, target->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a target parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the target "Name" is already defined.\n");       \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "direction")) {
      SETUP_MASK(POLICY, "policy");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, direction);
    } else if(!strcmp((char*)key->data.scalar.value, "position")) {
      SETUP_MASK(POLICY, "policy");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, position);
    } else if(!strcmp((char*)key->data.scalar.value, "sun")) {
      SETUP_MASK(POLICY, "policy");
      res = parse_sun(parser, doc, val);
    } else {
      log_err(parser, key, "unknown target parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  if(!(mask & BIT(POLICY))) {
    log_err(parser, target, "the target policy is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_pivot
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* pivot)
{
  enum { NORMAL, POINT, TARGET, TRANSFORM };
  double point[3];
  double normal[3];
  double translation[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && pivot);

  if(pivot->type != YAML_MAPPING_NODE) {
    log_err(parser, pivot, "expect a pivot definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = pivot->data.mapping.pairs.top - pivot->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, pivot->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, pivot->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect pivot parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the pivot parameter `"Name"' is already defined.\n");               \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "point")) {
      SETUP_MASK(POINT, "point");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, point);
    } else if(!strcmp((char*)key->data.scalar.value, "normal")) {
      SETUP_MASK(NORMAL, "normal");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, normal);
    } else if(!strcmp((char*)key->data.scalar.value, "target")) {
      SETUP_MASK(TARGET, "target");
      res = parse_target(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(parser, doc, val, translation, rotation);
    } else {
      log_err(parser, key, "unknown pivot parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, pivot, "the pivot parameter `"Name"' is missing.\n");    \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(POINT, "point");
  CHECK_PARAM(NORMAL, "normal");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Sun
 ******************************************************************************/
static res_T
parse_buie
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* buie)
{
  enum { CSR };
  intptr_t i, n;
  double csr;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && buie);

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
      res = parse_real(parser, val, nextafter(0, 1), nextafter(1, 0), &csr);
    } else {
      log_err(parser, key, "unknown buie parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
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
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* pillbox)
{
  enum { APERTURE };
  intptr_t i, n;
  double aperture;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && pillbox);

  if(pillbox->type != YAML_MAPPING_NODE) {
    log_err(parser, pillbox,
      "expect a pillbox definition of the sun radial angular distribution.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = pillbox->data.mapping.pairs.top - pillbox->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key = key;
    yaml_node_t* val = val;

    key = yaml_document_get_node(doc, pillbox->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, pillbox->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a pillbox parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "aperture")) {
      if(mask & BIT(APERTURE)) {
        log_err(parser, key, "the pillbox `aperture' is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(APERTURE);
      res = parse_real(parser, val, nextafter(0, 1), PI/2.0, &aperture);
    } else {
      log_err(parser, pillbox, "unknown pillbox parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

  if(!(mask & BIT(APERTURE))) {
    log_err(parser, pillbox, "the pillbox aperture parameter is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

res_T
parse_sun
  (struct solstice_parser* parser, yaml_document_t* doc, const yaml_node_t* sun)
{
  enum { DNI, RADIAL_ANGULAR_DISTRIB, SPECTRUM };
  double dni;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sun);

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
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &dni);
    } else if(!strcmp((char*)key->data.scalar.value, "buie")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      res = parse_buie(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "pillbox")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      res = parse_pillbox(parser, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "spectrum")) {
      SETUP_MASK(SPECTRUM, "spectrum");
      res = parse_spectrum(parser, doc, 0, DBL_MAX, val);
    } else {
      log_err(parser, key, "unknown sun parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, sun, "the sun "Name" is missing.\n");                    \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(DNI, "dni");
  CHECK_PARAM(SPECTRUM, "spectrum");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Item
 ******************************************************************************/
static res_T
parse_item
  (struct solstice_parser* parser,
   yaml_document_t* doc,
   const yaml_node_t* item)
{
  yaml_node_t* key;
  yaml_node_t* val;
  intptr_t n;
  res_T res = RES_OK;
  ASSERT(doc && item);

  if(item->type != YAML_MAPPING_NODE) {
    log_err(parser, item, "expect an item definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = item->data.mapping.pairs.top - item->data.mapping.pairs.start;
  if(n != 1) {
    log_err(parser, item,
      "expect only one \"key:value\" pair while %li are provided.\n", n);
    res = RES_BAD_ARG;
    goto error;
  }

  key = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].key);
  val = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].value);
  if(key->type != YAML_SCALAR_NODE) {
    log_err(parser, key, "expecting an item name.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)key->data.scalar.value, "instance")) {
    res = parse_instance(parser, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "material")) {
    res = parse_material(parser, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "node")) {
    res = parse_node(parser, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "object")) {
    res = parse_object(parser, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "pivot")) {
    res = parse_pivot(parser, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "sun")) {
    res = parse_sun(parser, doc, val);
  } else {
    log_err(parser, key, "unknown item `%s'.\n", key->data.scalar.value);
    res = RES_BAD_ARG;
  }
  if(res != RES_OK) goto error;

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_parser_create
  (struct mem_allocator* allocator, struct solstice_parser** out_parser)
{
  struct solstice_parser* parser = NULL;
  struct mem_allocator* mem_allocator;
  res_T res = RES_OK;
  ASSERT(out_parser);

  mem_allocator = allocator ? allocator : &mem_default_allocator;
  parser = MEM_CALLOC(mem_allocator, 1, sizeof(struct solstice_parser));
  if(!parser) {
    fprintf(stderr, "Could not allocat the Solstice parser.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  parser->allocator = mem_allocator;
  ref_init(&parser->ref);
  str_init(mem_allocator, &parser->stream_name);

exit:
  *out_parser = parser;
  return res;
error:
  if(parser) {
    solstice_parser_ref_put(parser);
    parser = NULL;
  }
  goto exit;
}

void
solstice_parser_ref_get(struct solstice_parser* parser)
{
  ASSERT(parser);
  ref_get(&parser->ref);
}

void
solstice_parser_ref_put(struct solstice_parser* parser)
{
  ASSERT(parser);
  ref_put(&parser->ref, parser_release);
}

res_T
solstice_parser_setup
  (struct solstice_parser* parser,
   const char* stream_name,
   FILE* stream)
{
  res_T res = RES_OK;
  ASSERT(parser && stream );

  if(parser->parser_is_init) {
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
  }
  res = str_set(&parser->stream_name, stream_name ? stream_name : "<stream>");
  if(res != RES_OK) {
    fprintf(stderr, "Could not register the filename.\n");
    goto error;
  }
  if(!yaml_parser_initialize(&parser->parser)) {
    fprintf(stderr, "Could not initialise the YAML parser.\n");
    res = RES_UNKNOWN_ERR;
    goto error;
  }
  parser->parser_is_init = 1;
  yaml_parser_set_input_file(&parser->parser, stream);

exit:
  return res;
error:
  str_clear(&parser->stream_name);
  if(parser->parser_is_init) {
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
  }
  goto exit;
}

res_T
solstice_parser_load(struct solstice_parser* parser)
{
  yaml_document_t doc;
  yaml_node_t* root;
  const char* filename;
  intptr_t i, n;
  int doc_is_init = 0;
  res_T res = RES_OK;
  ASSERT(parser);

  filename = str_cget(&parser->stream_name);

  if(!parser->parser_is_init) {
    res = RES_BAD_OP;
    goto error;
  }

  if(!yaml_parser_load(&parser->parser, &doc)) {
    fprintf(stderr, "%s:%lu:%lu: %s.\n",
      filename,
      (unsigned long)parser->parser.problem_mark.line+1,
      (unsigned long)parser->parser.problem_mark.column+1,
      parser->parser.problem);
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }
  doc_is_init = 1;

  root = yaml_document_get_root_node(&doc);
  if(!root) {
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }

  if(root->type != YAML_SEQUENCE_NODE) {
    log_err(parser, root, "expect a list of items.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = root->data.sequence.items.top - root->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* item;

    item = yaml_document_get_node(&doc, root->data.sequence.items.start[i]);
    res = parse_item(parser, &doc, item);
    if(res != RES_OK) goto error;
  }

exit:
  if(doc_is_init) yaml_document_delete(&doc);
  return res;
error:
  goto exit;
}

