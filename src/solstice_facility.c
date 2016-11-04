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

#include "solstice_facility.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

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
  ASSERT(node && fmt);

  fprintf(stderr, "%s:%lu:%lu: ",
    filename,
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
  va_start(vargs_list, fmt);
  vfprintf(stderr, fmt, vargs_list);
  va_end(vargs_list);
}

/*******************************************************************************
 * Material
 ******************************************************************************/
static res_T
parse_material_matte
  (const char* filename, yaml_document_t* doc, const yaml_node_t* matte)
{
  yaml_node_t* key;
  yaml_node_t* val;
  intptr_t n;
  res_T res = RES_OK;
  ASSERT(matte->type == YAML_MAPPING_NODE);

  n = matte->data.mapping.pairs.top - matte->data.mapping.pairs.start;
  if(n != 1) {
    log_err(filename, matte,
      "expect only one matte material attribute while %li are submitted.\n", n);
    res = RES_BAD_ARG;
    goto error;
  }

  key = yaml_document_get_node(doc, matte->data.mapping.pairs.start[0].key);
  val = yaml_document_get_node(doc, matte->data.mapping.pairs.start[0].value);
  if(key->type != YAML_SCALAR_NODE) {
    log_err(filename, key, "expect a matte material attribute.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(strcmp((char*)key->data.scalar.value, "reflectivity")) {
    log_err(filename, key, "unknown matte attribute `%s'.\n",
      key->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

  (void)val; /* TODO parse it */

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
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(mirror->type == YAML_MAPPING_NODE);

  n = mirror->data.mapping.pairs.top - mirror->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, mirror->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, mirror->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(filename, key,"expect mirror material attributes.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    (void)val; /* TODO parse it */

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
      SETUP_MASK(REFLECTIVITY, "reflectivity"); /* TODO parse the reflectivity  */
    } else if(!strcmp((char*)key->data.scalar.value, "roughness")) {
      SETUP_MASK(ROUGHNESS, "roughness"); /* TODO parse the roughness */
    } else {
      log_err(filename, key, "unknown mirror attribute `%s'.\n",
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
parse_material_descriptor
  (const char* filename, yaml_document_t* doc, const yaml_node_t* desc)
{
  yaml_node_t* key;
  yaml_node_t* val;
  intptr_t n;
  res_T res = RES_OK;
  ASSERT(desc && desc->type == YAML_MAPPING_NODE);

  n = desc->data.mapping.pairs.top - desc->data.mapping.pairs.start;
  if(n != 1) {
    log_err(filename, desc, "expect only one material descriptor.\n", n);
    res = RES_BAD_ARG;
    goto error;
  }

  key = yaml_document_get_node(doc, desc->data.mapping.pairs.start[0].key);
  val = yaml_document_get_node(doc, desc->data.mapping.pairs.start[0].value);
  if(key->type != YAML_SCALAR_NODE) {
    log_err(filename, key, "expect a material name.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)key->data.scalar.value, "matte")) {
    res = parse_material_matte(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "mirror")) {
    res = parse_material_mirror(filename, doc, val);
  } else {
    log_err(filename, key, "unknown material `%s'.\n", key->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }
  if(res != RES_OK) goto error;

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
  ASSERT(mtl && mtl->type == YAML_MAPPING_NODE);

  n = mtl->data.mapping.pairs.top - mtl->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, mtl->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, mtl->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(filename, key,
        "expect a material or a double sided material definition.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(filename, key, "the "Name" material is already defined.\n");   \
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
    #undef SETUP_MASK
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Object
 ******************************************************************************/
static res_T
parse_object
  (const char* filename, yaml_document_t* doc, const yaml_node_t* object)
{
  enum { MATERIAL, SHAPE, TRANSFORM };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(object && object->type == YAML_MAPPING_NODE);

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
      SETUP_MASK(TRANSFORM, "transform"); /* TODO parse the transform */
    } else {
      log_err(filename, key, "unknown object attribute `%s'.\n",
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
  ASSERT(item && item->type == YAML_MAPPING_NODE);

  n = item->data.mapping.pairs.top - item->data.mapping.pairs.start;
  if(n != 1) {
    log_err(filename, item,
      "expect only one `key : value' pair while %li are submitted.\n", n);
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

  if(!strcmp((char*)key->data.scalar.value, "material")) {
    res = parse_material(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "node")) { /* TODO */
  } else if(!strcmp((char*)key->data.scalar.value, "object")) {
    res = parse_object(filename, doc, val);
  } else if(!strcmp((char*)key->data.scalar.value, "pivot")) { /* TODO */
  } else if(!strcmp((char*)key->data.scalar.value, "spawn")) { /* TODO */
  } else {
    log_err(filename, key, "unknown item `%s'.\n", key->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

  if(res != RES_OK)
    goto error;

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
  size_t i, n;
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

  n = (size_t)(root->data.sequence.items.top - root->data.sequence.items.start);
  FOR_EACH(i, 0, n) {
    yaml_node_t* item;

    item = yaml_document_get_node(&doc, root->data.sequence.items.start[i]);
    if(item->type != YAML_MAPPING_NODE) {
      log_err(filename, item, "expect an item definition.\n");
      res = RES_BAD_ARG;
      goto error;
    }

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

