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

#include "solparser_c.h"

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static res_T
parse_anchor_alias
  (struct solparser* parser,
   const yaml_node_t* alias,
   const struct solparser_pivot_id pivot,
   struct solparser_anchor_id* out_ianchor)
{
  const struct solparser_anchor* anchor = NULL;
  intptr_t ianchor = INTPTR_MAX;
  res_T res = RES_OK;
  ASSERT(parser && alias && out_ianchor);

  if(alias->type != YAML_SCALAR_NODE) {
    log_err(parser, alias, "expect an anchor idententifier.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strncmp((char*)alias->data.scalar.value, "self.", 5)) {
    struct target_alias tgt;
    tgt.pivot = pivot;
    tgt.alias = alias;
    res = darray_tgtalias_push_back(&parser->tgtaliases, &tgt);
    if(res != RES_OK) {
      log_err(parser, alias, "could not register the anchor alias.\n");
      goto error;
    }
  } else {
    anchor = solparser_find_anchor(parser, (char*)alias->data.scalar.value);
    if(!anchor) {
      log_err(parser, alias, "undefined anchor `%s'.\n",
        alias->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }

    ianchor = anchor - darray_anchor_cdata_get(&parser->anchors);
    ASSERT(ianchor >= 0);
    ASSERT((size_t)ianchor < darray_anchor_size_get(&parser->anchors));
  }

exit:
  out_ianchor->i = (size_t)ianchor;
  return res;
error:
  ianchor = INTPTR_MAX;
  goto exit;
}

static res_T
parse_target
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* target_node,
   struct solparser_target* target,
   struct solparser_pivot_id pivot_id)
{
  enum { POLICY };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && target_node && target);

  if(target_node->type != YAML_MAPPING_NODE) {
    log_err(parser, target_node, "expect a target definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = target_node->data.mapping.pairs.top - target_node->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, target_node->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, target_node->data.mapping.pairs.start[i].value);
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
    if(!strcmp((char*)key->data.scalar.value, "anchor")) {
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_ANCHOR;
      res = parse_anchor_alias(parser, val, pivot_id, &target->data.anchor);
    } else if(!strcmp((char*)key->data.scalar.value, "direction")) {
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_DIRECTION;
      res = parse_real3
        (parser, doc, val, -DBL_MAX, DBL_MAX, target->data.direction);
    } else if(!strcmp((char*)key->data.scalar.value, "position")) {
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_POSITION;
      res = parse_real3
        (parser, doc, val, -DBL_MAX, DBL_MAX, target->data.position);
    } else if(!strcmp((char*)key->data.scalar.value, "sun")) {
      /* There is only one sun per YAML file. It is thus sufficient to define
       * the target_type to SOLPARSER_TARGET_SUN to indentify which data is
       * targeted, i.e. it is not necessary to store the identifier of the sun
       * to target */
      struct solparser_sun* sun;
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_SUN;
      res = parse_sun(parser, doc, val, &sun);
    } else {
      log_err(parser, key, "unknown target parameter `%s'.\n",
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

  if(!(mask & BIT(POLICY))) {
    log_err(parser, target_node, "the target policy is missing.\n");
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
parse_x_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* x_pivot,
   struct solparser_pivot_id* out_isolpivot)
{
  enum { REF_POINT, TARGET };
  struct solparser_x_pivot* solxpivot = NULL;
  size_t isolpivot = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && x_pivot && out_isolpivot);

  if(x_pivot->type != YAML_MAPPING_NODE) {
    log_err(parser, x_pivot, "expect a x_pivot definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the solstice pivot */
  isolpivot = darray_x_pivot_size_get(&parser->x_pivots);
  res = darray_x_pivot_resize(&parser->x_pivots, isolpivot + 1);
  if(res != RES_OK) {
    log_err(parser, x_pivot, "could not allocate the x_pivot.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  solxpivot = darray_x_pivot_data_get(&parser->x_pivots) + isolpivot;

  n = x_pivot->data.mapping.pairs.top - x_pivot->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, x_pivot->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, x_pivot->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect x_pivot parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the x_pivot parameter `"Name"' is already defined.\n");             \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "ref_point")) {
      SETUP_MASK(REF_POINT, "point");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, solxpivot->ref_point);
    } else if(!strcmp((char*)key->data.scalar.value, "target")) {
      struct solparser_pivot_id pivot_id;
      pivot_id.i = (size_t) (solxpivot - darray_x_pivot_cdata_get(&parser->x_pivots));
      SETUP_MASK(TARGET, "target");
      res = parse_target(parser, doc, val, &solxpivot->target, pivot_id);
    } else {
      log_err(parser, key, "unknown x_pivot parameter `%s'.\n",
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
      log_err(parser, x_pivot, "the x_pivot parameter `"Name"' is missing.\n");\
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(TARGET, "target");
  #undef CHECK_PARAM

  #define DEFAULT_PARAM(Flag, Ptr, Value)                                      \
    if(!(mask & BIT(Flag))) {                                                  \
      *(Ptr) = Value;                                                          \
    } (void)0
  DEFAULT_PARAM(REF_POINT, solxpivot->ref_point, 0);
  DEFAULT_PARAM(REF_POINT, solxpivot->ref_point+1, 0);
  DEFAULT_PARAM(REF_POINT, solxpivot->ref_point+2, 0);
  #undef DEFAULT_PARAM

exit:
  out_isolpivot->i = isolpivot;
  return res;
error:
  if(solxpivot) {
    darray_x_pivot_pop_back(&parser->x_pivots);
    isolpivot = SIZE_MAX;
  }
  goto exit;
}

res_T
parse_zx_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* zx_pivot,
   struct solparser_pivot_id* out_isolpivot)
{
  enum { SPACING, REF_POINT, TARGET };
  struct solparser_zx_pivot* solxzpivot = NULL;
  size_t isolpivot = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && zx_pivot && out_isolpivot);

  if(zx_pivot->type != YAML_MAPPING_NODE) {
    log_err(parser, zx_pivot, "expect a zx_pivot definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the solstice pivot */
  isolpivot = darray_zx_pivot_size_get(&parser->zx_pivots);
  res = darray_zx_pivot_resize(&parser->zx_pivots, isolpivot + 1);
  if(res != RES_OK) {
    log_err(parser, zx_pivot, "could not allocate the zx_pivot.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  solxzpivot = darray_zx_pivot_data_get(&parser->zx_pivots) + isolpivot;

  n = zx_pivot->data.mapping.pairs.top - zx_pivot->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, zx_pivot->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(
      doc, zx_pivot->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect zx_pivot parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the zx_pivot parameter `"Name"' is already defined.\n");            \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*) key->data.scalar.value, "spacing")) {
      SETUP_MASK(SPACING, "spacing");
      res = parse_real(parser, val, 0, DBL_MAX, &solxzpivot->spacing);
    } else if(!strcmp((char*) key->data.scalar.value, "ref_point")) {
      SETUP_MASK(REF_POINT, "ref_point");
      res = parse_real3(
        parser, doc, val, -DBL_MAX, DBL_MAX, solxzpivot->ref_point);
    } else if(!strcmp((char*) key->data.scalar.value, "target")) {
      struct solparser_pivot_id pivot_id;
      pivot_id.i =
        (size_t) (solxzpivot - darray_zx_pivot_cdata_get(&parser->zx_pivots));
      SETUP_MASK(TARGET, "target");
      res = parse_target(parser, doc, val, &solxzpivot->target, pivot_id);
    } else {
      log_err(parser, key, "unknown zx_pivot parameter `%s'.\n",
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
      log_err(parser, zx_pivot,                                                \
         "the zx_pivot parameter `"Name"' is missing.\n");                     \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(TARGET, "target");
  #undef CHECK_PARAM

  #define DEFAULT_PARAM(Flag, Ptr, Value)                                      \
    if(!(mask & BIT(Flag))) {                                                  \
      *(Ptr) = Value;                                                          \
    } (void)0
  DEFAULT_PARAM(SPACING, &solxzpivot->spacing, 0);
  DEFAULT_PARAM(REF_POINT, solxzpivot->ref_point, 0);
  DEFAULT_PARAM(REF_POINT, solxzpivot->ref_point+1, 0);
  DEFAULT_PARAM(REF_POINT, solxzpivot->ref_point+2, 0);
  #undef DEFAULT_PARAM

exit:
  out_isolpivot->i = isolpivot;
  return res;
error:
  if(solxzpivot) {
    darray_zx_pivot_pop_back(&parser->zx_pivots);
    isolpivot = SIZE_MAX;
  }
  goto exit;
}


