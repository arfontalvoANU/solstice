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

#include "solparser_c.h"

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static res_T
entity_register_name
  (struct solparser* parser,
   const yaml_node_t* entity,
   struct htable_str2sols* htable,
   const size_t isolent)
{
  struct solparser_entity* solent;
  size_t* pisolent;
  res_T res = RES_OK;
  ASSERT(parser && htable);
  ASSERT(isolent < darray_entity_size_get(&parser->entities));

  solent = darray_entity_data_get(&parser->entities) + isolent;

  pisolent = htable_str2sols_find(htable, &solent->name);
  if(pisolent) {
    log_err(parser, entity,
      "an entity with the name `%s' is already defined in the current context.\n",
      str_cget(&solent->name));
    return RES_BAD_ARG;
  }

  res = htable_str2sols_set(htable, &solent->name, &isolent);
  if(res != RES_OK) {
    log_err(parser, entity, "could not register the entity.\n");
    return res;
  }
  return RES_OK;
}

static res_T
anchor_register_name
  (struct solparser* parser,
   const yaml_node_t* anchor,
   struct htable_str2sols* htable,
   const size_t isolanchor)
{
  struct solparser_anchor* solanchor;
  size_t* pisolanchor;
  res_T res = RES_OK;
  ASSERT(parser && htable);
  ASSERT(isolanchor < darray_anchor_size_get(&parser->anchors));

  solanchor = darray_anchor_data_get(&parser->anchors) + isolanchor;

  pisolanchor = htable_str2sols_find(htable, &solanchor->name);
  if(pisolanchor) {
    log_err(parser, anchor,
      "an anchor with the name `%s' is already defined in the cunrrent context.\n",
      str_cget(&solanchor->name));
    return RES_BAD_ARG;
  }

  res = htable_str2sols_set(htable, &solanchor->name, &isolanchor);
  if(res != RES_OK) {
    log_err(parser, anchor, "could not register the anchor.\n");
    return res;
  }
  return RES_OK;
}

static res_T
parse_identifier_string
  (struct solparser* parser,
   yaml_node_t* name,
   struct str* str)
{
  res_T res = RES_OK;
  ASSERT(parser && name && str);

  res = parse_string(parser, name, str);
  if(res != RES_OK) goto error;

  if(strchr(str_cget(str), '.')) {
    log_err(parser, name, "invalid character `.' in the name `%s'.\n",
      str_cget(str));
    res = RES_BAD_ARG;
    goto error;
  }
  if(strchr(str_cget(str), ' ') || strchr(str_cget(str), '\t')) {
    log_err(parser, name, "invalid space or tabulation in the name `%s'.\n",
      str_cget(str));
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_anchor
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* anchor,
   struct htable_str2sols* htable,
   struct solparser_anchor_id* out_isolanchor)
{
  enum { NAME, POSITION };
  struct solparser_anchor* solanchor = NULL;
  size_t isolanchor = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(parser && anchor && out_isolanchor);

  if(anchor->type != YAML_MAPPING_NODE) {
    log_err(parser, anchor, "expect an anchor definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the anchor */
  isolanchor = darray_anchor_size_get(&parser->anchors);
  res = darray_anchor_resize(&parser->anchors, isolanchor + 1);
  if(res != RES_OK) {
    log_err(parser, anchor, "could not allocate the anchor.\n");
    goto error;
  }
  solanchor = darray_anchor_data_get(&parser->anchors) + isolanchor;

  n = anchor->data.mapping.pairs.top - anchor->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, anchor->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, anchor->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an anchor attribute.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the anchor "Name" is already defined.\n");       \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "name")) {
      SETUP_MASK(NAME, "name");
      res = parse_identifier_string(parser, val, &solanchor->name);
    } else if(!strcmp((char*)key->data.scalar.value, "position")) {
      SETUP_MASK(POSITION, "position description");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, solanchor->position);
    } else if(!strcmp((char*) key->data.scalar.value, "hyperboloid_image_focals")) {
      struct solparser_hyperboloid_focals focals;
      SETUP_MASK(POSITION, "position description");
      res = parse_focals_description(parser, doc, val, &focals);
      if(res != RES_OK) goto error;
      d3(solanchor->position, 0, 0, focals.image);
    } else {
      log_err(parser, key, "unknown anchor parameter `%s'.\n",
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
      log_err(parser, anchor, "the anchor "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(NAME, "name");
  CHECK_PARAM(POSITION, "position description");
  #undef CHECK_PARAM

  res = anchor_register_name(parser, anchor, htable, isolanchor);
  if(res != RES_OK) goto error;

exit:
  out_isolanchor->i = isolanchor;
  return res;
error:
  if(solanchor) {
    darray_anchor_pop_back(&parser->anchors);
    isolanchor = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_anchors
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* anchors,
   struct htable_str2sols* htable,
   struct darray_anchor_id* solanchors)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(parser && anchors);

  if(anchors->type != YAML_SEQUENCE_NODE) {
    log_err(parser, anchors, "expect a list of anchors.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = anchors->data.sequence.items.top - anchors->data.sequence.items.start;
  res = darray_anchor_id_resize(solanchors, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, anchors, "could not allocate the anchors list.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    struct solparser_anchor_id* anchor_id;
    yaml_node_t* anchor;

    anchor_id = darray_anchor_id_data_get(solanchors)+i;
    anchor = yaml_document_get_node(doc, anchors->data.sequence.items.start[i]);
    res = parse_anchor(parser, doc, anchor, htable, anchor_id);
    if(res != RES_OK) goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_children
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* children,
   struct htable_str2sols* htable,
   struct darray_child_id* entities)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(parser && children && htable && entities);

  if(children->type != YAML_SEQUENCE_NODE) {
    log_err(parser, children, "expect a list of entities.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = children->data.sequence.items.top - children->data.sequence.items.start;
  res = darray_child_id_resize(entities, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, children, "could not allocate the children list.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    struct solparser_entity_id* entity_id = darray_child_id_data_get(entities) + i;
    yaml_node_t* child;

    child = yaml_document_get_node(doc, children->data.sequence.items.start[i]);
    res = parse_entity(parser, doc, child, htable, entity_id);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  darray_child_id_clear(entities);
  goto exit;
}


/*******************************************************************************
 * Local function
 ******************************************************************************/
res_T
parse_entity
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* entity,
   struct htable_str2sols* htable,
   struct solparser_entity_id* out_isolent)
{
  enum { ANCHORS, CHILDREN, DATA, NAME, TRANSFORM, PRIMARY };
  struct solparser_entity solent;
  struct solparser_entity* psolent;
  size_t isolent = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && entity && htable && out_isolent);

  solparser_entity_init(parser->allocator, &solent);

  if(entity->type != YAML_MAPPING_NODE) {
    log_err(parser, entity, "expect an entity definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the entity but *DO NOT* retrieve a pointer onto it since the
   * allocation of its children may update its memory location. Use the "on
   * stack" entity `solent' instead. */
  isolent = darray_entity_size_get(&parser->entities);
  res = darray_entity_resize(&parser->entities, isolent + 1);
  if(res != RES_OK) {
    log_err(parser, entity, "could not allocate the entity.\n");
    goto error;
  }

  n = entity->data.mapping.pairs.top - entity->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, entity->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, entity->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an entity attribute.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the entity "Name" is already defined.\n");                          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "anchors")) {
      SETUP_MASK(ANCHORS, "anchors");
      res = parse_anchors
        (parser, doc, val, &solent.str2anchors, &solent.anchors);
    } else if(!strcmp((char*)key->data.scalar.value, "children")) {
      SETUP_MASK(CHILDREN, "children");
      res = parse_children
        (parser, doc, val, &solent.str2children, &solent.children);
    } else if(!strcmp((char*)key->data.scalar.value, "geometry")) {
      SETUP_MASK(DATA, "data");
      solent.type = SOLPARSER_ENTITY_GEOMETRY;
      res = parse_geometry(parser, doc, val, &solent.data.geometry);
    } else if(!strcmp((char*)key->data.scalar.value, "name")) {
      SETUP_MASK(NAME, "name");
      res = parse_identifier_string(parser, val, &solent.name);
      if(!strcmp(str_get(&solent.name), "self")) {
        /* Self is a reserved keyword */
        log_err(parser, key, "Reserved keywords cannot be used as names: %s.\n",
          str_get(&solent.name));
        res = RES_BAD_ARG;
        goto error;
      }
    } else if(!strcmp((char*)key->data.scalar.value, "x_pivot")) {
      SETUP_MASK(DATA, "data");
      solent.type = SOLPARSER_ENTITY_X_PIVOT;
      res = parse_x_pivot(parser, doc, val, &solent.data.x_pivot);
    } else if(!strcmp((char*) key->data.scalar.value, "zx_pivot")) {
      SETUP_MASK(DATA, "data");
      solent.type = SOLPARSER_ENTITY_ZX_PIVOT;
      res = parse_zx_pivot(parser, doc, val, &solent.data.zx_pivot);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform
        (parser, doc, val, solent.translation, solent.rotation);
    } else if(!strcmp((char*) key->data.scalar.value, "primary")) {
      long tmp;
      SETUP_MASK(PRIMARY, "primary");
      /* FIXME: add NONE/ FRONT / BACK / FRONT_AND_BACK qualifier
       * to avoid a misunderstanding about shadows results */
      res = parse_integer(parser, val, 0, 1, &tmp);
      solent.primary = (int)tmp;
    } else {
      log_err(parser, key, "unknown entity parameter `%s'.\n",
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

  if(!(mask & BIT(DATA))) {
    solent.type = SOLPARSER_ENTITY_EMPTY;
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, entity, "the entity "Name" parameter is missing.\n");    \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(NAME, "name");
  if(solent.type == SOLPARSER_ENTITY_GEOMETRY) {
    CHECK_PARAM(PRIMARY, "primary");
  } else if(mask & BIT(PRIMARY)) {
    log_err(parser, entity,
      "the entity primary parameter is invalid in this context.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  #undef CHECK_PARAM

  psolent = darray_entity_data_get(&parser->entities) + isolent;
  res = solparser_entity_copy_and_clear(psolent, &solent);
  if(res != RES_OK) {
    log_err(parser, entity,
      "could not copy the loaded entity into the parser data structures.\n");
    goto error;
  }
  res = entity_register_name(parser, entity, htable, isolent);
  if(res != RES_OK) goto error;

exit:
  solparser_entity_release(&solent);
  out_isolent->i = isolent;
  return res;
error:
  if(isolent != SIZE_MAX) {
    htable_str2sols_erase(htable, &solent.name);
    darray_entity_pop_back(&parser->entities);
    isolent = SIZE_MAX;
  }
  goto exit;
}


