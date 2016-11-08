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

#define _POSIX_C_SOURCE 200112L

#include "solstice_facility.h"

#include <rsys/cstr.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

static res_T
parse_node
  (const char* filename,
   yaml_document_t* doc,
   const yaml_node_t* children);

static res_T
parse_object
  (const char* filename,
   yaml_document_t* doc,
   const yaml_node_t* object);

static res_T
parse_pivot
  (const char* filename,
   yaml_document_t* doc,
   const yaml_node_t* pivot);

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static INLINE void
log_err
  (const char* filename,
   const yaml_node_t* node,
   const char* fmt,
   ...)
{
  va_list vargs_list;
  ASSERT(filename && node && fmt);

  fprintf(stderr, "%s:%lu:%lu: ",
    filename,
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
  va_start(vargs_list, fmt);
  vfprintf(stderr, fmt, vargs_list);
  va_end(vargs_list);
}

/*******************************************************************************
 * Miscellaneous parsing functions
 ******************************************************************************/
static res_T
parse_real
  (const char* filename,
   const yaml_node_t* real,
   const double lower_bound,
   const double upper_bound,
   double* dst)
{
  res_T res = RES_OK;
  ASSERT(real && dst);
  ASSERT(lower_bound < upper_bound);

  if(real->type != YAML_SCALAR_NODE) {
    log_err(filename, real, "expect a floating point number.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  res = cstr_to_double((char*)real->data.scalar.value, dst);
  if(res != RES_OK) {
    log_err(filename, real, "invalid floatin point number `%s'.\n",
      real->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

  if(*dst < lower_bound || *dst > upper_bound) {
    log_err(filename, real, "%g must be in [%g, %g].\n",
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
  (const char* filename,
   yaml_document_t* doc,
   const yaml_node_t* real3,
   double dst[3])
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && real3 && dst);

  if(real3->type != YAML_SEQUENCE_NODE) {
    log_err(filename, real3, "expect a sequence of 3 reals.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = real3->data.sequence.items.top - real3->data.sequence.items.start;
  if(n != 3) {
    log_err(filename, real3, "expect 3 reals while `%li' %s submitted.\n",
      n, n > 1 ? "are" : "is");
    res = RES_BAD_ARG;
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* real;
    real = yaml_document_get_node(doc, real3->data.sequence.items.start[i]);
    res = parse_real(filename, real,-DBL_MAX, DBL_MAX, dst + i);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_transform
  (const char* filename,
   yaml_document_t* doc,
   const yaml_node_t* transform,
   double position[3],
   double rotation[3])
{
  enum { POSITION, ROTATION };
  intptr_t i, n;
  int mask = 0;
  res_T res = RES_OK;
  ASSERT(doc && position && rotation && transform);

  if(transform->type != YAML_MAPPING_NODE) {
    log_err(filename, transform, "expect a mapping of transform attributes.\n");
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
      log_err(filename, key, "expect transform parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key, "the transform `"Name"' is already defined.\n");\
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "position")) {
      SETUP_MASK(POSITION, "position");
      res = parse_real3(filename, doc, val, position);
    } else if(!strcmp((char*)key->data.scalar.value, "rotation")) {
      SETUP_MASK(ROTATION, "rotation");
      res = parse_real3(filename, doc, val, rotation);
    } else {
      log_err(filename, key, "unknown transform parameter `%s'.\n",
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
  (const char* filename, yaml_document_t* doc,
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
    log_err(filename, sdata, "expect a the definition of a spectrum data.\n");
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
      log_err(filename, key, "expect a spectrum data parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key,                                                 \
          "the `"Name"' of the spectrum data is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "data")) {
      SETUP_MASK(DATA, "data");
      res = parse_real(filename, val, lower_bound, upper_bound, &data);
    } else if(!strcmp((char*)key->data.scalar.value, "wavelength")) {
      SETUP_MASK(WAVELENGTH, "wavelength");
      res = parse_real(filename, val, 0, DBL_MAX, &wavelength);
    } else {
      log_err(filename, key, "unknown spectrum data parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(filename, sdata,"the "Name" of the spectrum data is missing.\n");\
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
  (const char* filename,
   yaml_document_t* doc,
   const double lower_bound,
   const double upper_bound,
   const yaml_node_t* spectrum)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && spectrum && lower_bound < upper_bound);

  if(spectrum->type != YAML_SEQUENCE_NODE) {
    log_err(filename, spectrum, "expect a list of spectrum data.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = spectrum->data.sequence.items.top - spectrum->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* sdata;

    sdata = yaml_document_get_node(doc, spectrum->data.sequence.items.start[i]);
    res = parse_spectrum_data(filename, doc, lower_bound, upper_bound, sdata);
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
  (const char* filename, yaml_document_t* doc, const yaml_node_t* inst)
{
  enum { GEOMETRY, TRANSFORM };
  double position[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  intptr_t i, n;
  int mask = 0;
  res_T res = RES_OK;
  ASSERT(doc && inst);

  if(inst->type != YAML_MAPPING_NODE) {
    log_err(filename, inst, "expect an instance definition.\n");
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
      log_err(filename, key, "expect instance parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key, "the instance "Name" is already defined.\n");   \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "node")) {
      SETUP_MASK(GEOMETRY, "geometry");
      res = parse_node(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "object")) {
      SETUP_MASK(GEOMETRY, "geometry");
      res = parse_object(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(filename, doc, val, position, rotation);
    } else {
      log_err(filename, key, "unknown instance parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(filename, inst, "the instance "Name" is missing.\n");            \
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
  (const char* filename, yaml_document_t* doc, const yaml_node_t* matte)
{
  enum { REFLECTIVITY };
  double reflectivity;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && matte);

  if(matte->type != YAML_MAPPING_NODE) {
    log_err(filename, matte, "expect a mapping of matte material attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = matte->data.mapping.pairs.top - matte->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, matte->data.mapping.pairs.start[0].key);
    val = yaml_document_get_node(doc, matte->data.mapping.pairs.start[0].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(filename, key, "expect a matte material attribute.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      if(mask & BIT(REFLECTIVITY)) {
        log_err(filename, key, "the matte reflectivity is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(REFLECTIVITY);
      res = parse_real(filename, val, 0, 1, &reflectivity);
    } else {
      log_err(filename, key, "unknown matte attribute `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

  if(!(mask & BIT(REFLECTIVITY))) {
    log_err(filename, matte, "the matte reflectivity is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* TODO create the Solstice material */

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_material_mirror
  (const char* filename, yaml_document_t* doc, const yaml_node_t* mirror)
{
  enum { REFLECTIVITY, ROUGHNESS };
  double reflectivity;
  double roughness;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && mirror);

  if(mirror->type != YAML_MAPPING_NODE) {
    log_err(filename, mirror,
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
      log_err(filename, key, "expect a mirror material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key,                                                 \
          "the "Name" of the mirror material is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      SETUP_MASK(REFLECTIVITY, "reflectivity");
      res = parse_real(filename, val, 0, 1, &reflectivity);
    } else if(!strcmp((char*)key->data.scalar.value, "roughness")) {
      SETUP_MASK(ROUGHNESS, "roughness");
      res = parse_real(filename, val, 0, 1, &roughness);
    } else {
      log_err(filename, key, "unknown mirror attribute `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(filename, mirror, "the mirror "Name" is missing.\n");            \
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
  (const char* filename, yaml_document_t* doc, const yaml_node_t* desc)
{
  enum { DESCRIPTOR };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && desc);

  /* TODO If the descriptor is an alias of an already created material skip the
   * parsing and return the aliased material descriptor */

  if(desc->type != YAML_MAPPING_NODE) {
    log_err(filename, desc, "expect a material descriptor.\n");
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
      log_err(filename, key, "expect a material name.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key, "the material "Name" is already defined.\n");   \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "matte")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      res = parse_material_matte(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "mirror")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      res = parse_material_mirror(filename, doc, val);
    } else {
      log_err(filename, key, "unknown material descriptor `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  if(!(mask & BIT(DESCRIPTOR))) {
    log_err(filename, desc, "the material descriptor is missing.\n");
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
  (const char* filename, yaml_document_t* doc, const yaml_node_t* mtl)
{
  enum { FRONT, BACK };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && mtl);

  if(mtl->type != YAML_MAPPING_NODE) {
    log_err(filename, mtl, "expect a material definition.\n");
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
      log_err(filename, key,
        "expect a material descriptor or a double sided material.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key,                                                 \
          "the "Name" material descriptor is already defined.\n");             \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "front")) {
      SETUP_MASK(FRONT, "front");
      res = parse_material_descriptor(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "back")) {
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(filename, doc, val);
    } else {
      SETUP_MASK(FRONT, "front");
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(filename, doc, mtl);
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                             \
    if(!(mask & BIT(Flag))) {                                                 \
      log_err(filename, mtl, "the "Name" material descriptor is missing.\n"); \
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
 * Object
 ******************************************************************************/
res_T
parse_object
  (const char* filename, yaml_document_t* doc, const yaml_node_t* object)
{
  enum { MATERIAL, SHAPE, TRANSFORM };
  double position[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && object);

  if(object->type != YAML_MAPPING_NODE) {
    log_err(filename, object, "expect an object definition.\n");
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
      log_err(filename, key, "expect a object parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key,                                                 \
          "the object attribute `"Name"' is already defined.\n");              \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "material")) {
      SETUP_MASK(MATERIAL, "material");
      res = parse_material(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "cube")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "cylinder")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "obj")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "parabol")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "parabolic-cylinder")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "plane")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "sphere")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "stl")) {
      SETUP_MASK(SHAPE, "shape"); /* TODO parse the shape */
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(filename, doc, val, position, rotation);
    } else {
      log_err(filename, key, "unknown object attribute `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(filename, object, "the object attribute `"Name"' is missing.\n");\
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
  (const char* filename, yaml_document_t* doc, const yaml_node_t* children)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && children);

  if(children->type != YAML_SEQUENCE_NODE) {
    log_err(filename, children, "expect a list of nodes.\n");
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
      log_err(filename, child, "expect a node definition.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    nb = child->data.mapping.pairs.top - child->data.mapping.pairs.start;
    if(nb != 1) {
      log_err(filename, child,
        "expect only one \"key:value\" pair while %li are provided.\n", nb);
      res = RES_BAD_ARG;
      goto error;
    }

    key = yaml_document_get_node(doc, child->data.mapping.pairs.start[0].key);
    val = yaml_document_get_node(doc, child->data.mapping.pairs.start[0].value);
    if(!strcmp((char*)key->data.scalar.value, "node")) {
      res = parse_node(filename, doc, val);
    } else {
      log_err(filename, key, "unexpected directive `%s'. Expect a node.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_entities
  (const char* filename, yaml_document_t* doc, const yaml_node_t* entities)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && entities);

  if(entities->type != YAML_SEQUENCE_NODE) {
    log_err(filename, entities, "expect a list of entities.\n");
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
      log_err(filename, entity, "expect an entity definition.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    nb = entity->data.mapping.pairs.top - entity->data.mapping.pairs.start;
    if(nb != 1) {
      log_err(filename, entity,
        "expect only one \"key:value\" pair while %li are provided.\n", nb);
      res = RES_BAD_ARG;
      goto error;
    }

    key = yaml_document_get_node(doc, entity->data.mapping.pairs.start[0].key);
    val = yaml_document_get_node(doc, entity->data.mapping.pairs.start[0].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(filename, key, "expect an entity name.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    if(!strcmp((char*)key->data.scalar.value, "object")) {
      res = parse_object(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "pivot")) {
      res = parse_pivot(filename, doc, val);
    } else {
      log_err(filename, key, "unknown entity `%s'.\n", key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

res_T
parse_node(const char* filename, yaml_document_t* doc, const yaml_node_t* node)
{
  enum { CHILDREN, ENTITIES, TRANSFORM };
  double position[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  intptr_t i, n;
  int mask = 0;
  res_T res = RES_OK;
  ASSERT(doc && node);

  if(node->type != YAML_MAPPING_NODE) {
    log_err(filename, node, "expect a node definition.\n");
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
      log_err(filename, key, "expect a node attribute.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key,                                                 \
          "the node attribute `"Name"' is already defined.\n");                \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "children")) {
      SETUP_MASK(CHILDREN, "children");
      res = parse_children(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "entities")) {
      SETUP_MASK(ENTITIES, "entities");
      res = parse_entities(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(filename, doc, val, position, rotation);
    } else {
      log_err(filename, key, "unknown node attribute `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(filename, node, "the node attribute `"Name"' is missing.\n");    \
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
parse_pivot
  (const char* filename,
   yaml_document_t* doc,
   const yaml_node_t* pivot)
{
  enum { NORMAL, POINT, TARGET, TRANSFORM };
  double point[3];
  double normal[3];
  double position[3] = {0, 0, 0};
  double rotation[3] = {0, 0, 0};
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && pivot);

  if(pivot->type != YAML_MAPPING_NODE) {
    log_err(filename, pivot, "expect a pivot definition.\n");
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
      log_err(filename, key, "expect pivot parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key,                                                 \
          "the pivot parameter `"Name"' is already defined.\n");               \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "point")) {
      SETUP_MASK(POINT, "point");
      res = parse_real3(filename, doc, val, point);
    } else if(!strcmp((char*)key->data.scalar.value, "normal")) {
      SETUP_MASK(NORMAL, "normal");
      res = parse_real3(filename, doc, val, normal);
    } else if(!strcmp((char*)key->data.scalar.value, "target")) {
      SETUP_MASK(TARGET, "target"); /* TODO parse the target */
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(filename, doc, val, position, rotation);
    } else {
      log_err(filename, key, "unknown pivot parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(filename, pivot, "the pivot parameter `"Name"' is missing.\n");  \
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
parse_buie(const char* filename, yaml_document_t* doc, const yaml_node_t* buie)
{
  enum { CSR };
  intptr_t i, n;
  double csr;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && buie);

  if(buie->type != YAML_MAPPING_NODE) {
    log_err(filename, buie,
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
      log_err(filename, key, "expect a buie parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    if(!strcmp((char*)key->data.scalar.value, "csr")) {
      if(mask & BIT(CSR)) {
        log_err(filename, key, "the buie `csr' is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(CSR);
      res = parse_real(filename, val, nextafter(0, 1), nextafter(1, 0), &csr);
    } else {
      log_err(filename, key, "unknown buie parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

  if(!(mask & BIT(CSR))) {
    log_err(filename, buie, "the buie csr parameter is missing.\n");
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
  (const char* filename, yaml_document_t* doc, const yaml_node_t* pillbox)
{
  enum { APERTURE };
  intptr_t i, n;
  double aperture;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && pillbox);

  if(pillbox->type != YAML_MAPPING_NODE) {
    log_err(filename, pillbox,
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
      log_err(filename, key, "expect a pillbox parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "aperture")) {
      if(mask & BIT(APERTURE)) {
        log_err(filename, key, "the pillbox `aperture' is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(APERTURE);
      res = parse_real(filename, val, nextafter(0, 1), PI/2.0, &aperture);
    } else {
      log_err(filename, pillbox, "unknown pillbox parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
  }

  if(!(mask & BIT(APERTURE))) {
    log_err(filename, pillbox, "the pillbox aperture parameter is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_sun(const char* filename, yaml_document_t* doc, const yaml_node_t* sun)
{
  enum { DNI, RADIAL_ANGULAR_DISTRIB, SPECTRUM };
  double dni;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sun);

  if(sun->type != YAML_MAPPING_NODE) {
    log_err(filename, sun, "expect a sun definition.\n");
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
      log_err(filename, key, "expect sun parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key, "the sun "Name" is already defined.\n");        \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "dni")) {
      SETUP_MASK(DNI, "dni");
      res = parse_real(filename, val, nextafter(0, 1), DBL_MAX, &dni);
    } else if(!strcmp((char*)key->data.scalar.value, "buie")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      res = parse_buie(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "pillbox")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      res = parse_pillbox(filename, doc, val);
    } else if(!strcmp((char*)key->data.scalar.value, "spectrum")) {
      SETUP_MASK(SPECTRUM, "spectrum");
      res = parse_spectrum(filename, doc, 0, DBL_MAX, val);
    } else {
      log_err(filename, key, "unknown sun parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(filename, sun, "the sun "Name" is missing.\n");                  \
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
  (const char* filename, yaml_document_t* doc, const yaml_node_t* item)
{
  yaml_node_t* key;
  yaml_node_t* val;
  intptr_t n;
  res_T res = RES_OK;
  ASSERT(doc && item);

  if(item->type != YAML_MAPPING_NODE) {
    log_err(filename, item, "expect an item definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = item->data.mapping.pairs.top - item->data.mapping.pairs.start;
  if(n != 1) {
    log_err(filename, item,
      "expect only one \"key:value\" pair while %li are provided.\n", n);
    res = RES_BAD_ARG;
    goto error;
  }

  key = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].key);
  val = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].value);
  if(key->type != YAML_SCALAR_NODE) {
    log_err(filename, key, "expecting an item name.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)key->data.scalar.value, "instance")) {
    res = parse_instance(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "material")) {
    res = parse_material(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "node")) {
    res = parse_node(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "object")) {
    res = parse_object(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "pivot")) {
    res = parse_pivot(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "sun")) {
    res = parse_sun(filename, doc, val);
  } else {
    log_err(filename, key, "unknown item `%s'.\n", key->data.scalar.value);
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
solstice_facility_load(const char* filename)
{
  yaml_parser_t parser;
  yaml_document_t doc;
  yaml_node_t* root;
  FILE* file = NULL;
  intptr_t i, n;
  int doc_is_init;
  res_T res = RES_OK;

  if(!yaml_parser_initialize(&parser)) {
    fprintf(stderr, "Could not initialise the YAML parser.\n");
    return RES_UNKNOWN_ERR;
  }

  file = fopen(filename, "rb");
  if(!file) {
    fprintf(stderr, "Could not open the YAML file `%s'.\n", filename);
    res = RES_IO_ERR;
    goto error;
  }

  yaml_parser_set_input_file(&parser, file);

  if(!yaml_parser_load(&parser, &doc)) {
    fprintf(stderr, "%s:%lu:%lu: %s.\n",
      filename,
      (unsigned long)parser.problem_mark.line+1,
      (unsigned long)parser.problem_mark.column+1,
      parser.problem);
    res = RES_IO_ERR;
    goto error;
  }
  doc_is_init = 1;

  root = yaml_document_get_root_node(&doc);
  if(!root) {
    fprintf(stderr, "The file `%s' seems empty.\n", filename);
    res = RES_BAD_ARG;
    goto error;
  }
  if(root->type != YAML_SEQUENCE_NODE) {
    log_err(filename, root, "expect a list of items.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = root->data.sequence.items.top - root->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* item;

    item = yaml_document_get_node(&doc, root->data.sequence.items.start[i]);
    res = parse_item(filename, &doc, item);
    if(res != RES_OK) goto error;
  }

exit:
  yaml_parser_delete(&parser);
  if(doc_is_init) yaml_document_delete(&doc);
  if(file) fclose(file);
  return res;
error:
  goto exit;
}

