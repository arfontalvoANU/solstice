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

#ifndef SOLPARSER_ENTITY_H
#define SOLPARSER_ENTITY_H

#include "solparser_geometry.h"
#include "solparser_pivot.h"
#include "solparser_shape.h"

#include <rsys/double3.h>
#include <rsys/dynamic_array.h>
#include <rsys/hash_table.h>
#include <rsys/list.h>
#include <rsys/str.h>

enum solparser_entity_type {
  SOLPARSER_ENTITY_EMPTY,
  SOLPARSER_ENTITY_GEOMETRY,
  SOLPARSER_ENTITY_X_PIVOT,
  SOLPARSER_ENTITY_ZX_PIVOT
};

struct solparser_entity_id { size_t i; };

#define DARRAY_NAME child_id
#define DARRAY_DATA struct solparser_entity_id
#include <rsys/dynamic_array.h>

#define DARRAY_NAME anchor_id
#define DARRAY_DATA struct solparser_anchor_id
#include <rsys/dynamic_array.h>

/* Declare the hash table that map an entity name to the index of its in memory
 * solstice representation. */
#define HTABLE_NAME str2sols
#define HTABLE_KEY struct str
#define HTABLE_KEY_FUNCTOR_INIT str_init
#define HTABLE_KEY_FUNCTOR_RELEASE str_release
#define HTABLE_KEY_FUNCTOR_COPY str_copy
#define HTABLE_KEY_FUNCTOR_COPY_AND_RELEASE str_copy_and_release
#define HTABLE_KEY_FUNCTOR_EQ str_eq
#define HTABLE_KEY_FUNCTOR_HASH str_hash
#define HTABLE_DATA size_t
#include <rsys/hash_table.h>

struct solparser_entity {
  double rotation[3]; /* In degrees */
  double translation[3];

  struct str name;

  int primary;

  enum solparser_entity_type type;
  union {
    struct solparser_geometry_id geometry;
    struct solparser_pivot_id x_pivot;
    struct solparser_pivot_id zx_pivot;
  } data;

  /* Internal data. Should not be acceded directly. */
  struct htable_str2sols str2anchors;
  struct htable_str2sols str2children;
  struct darray_anchor_id anchors; /* List of anchors */
  struct darray_child_id children; /* List of children nodes */
};

static INLINE void
solparser_entity_init
  (struct mem_allocator* allocator, struct solparser_entity* entity)
{
  ASSERT(entity);
  d3_splat(entity->rotation, 0);
  d3_splat(entity->translation, 0);
  entity->type = SOLPARSER_ENTITY_GEOMETRY;
  entity->data.geometry.i = SIZE_MAX;
  str_init(allocator, &entity->name);
  entity->primary = 2;
  htable_str2sols_init(allocator, &entity->str2anchors);
  htable_str2sols_init(allocator, &entity->str2children);
  darray_anchor_id_init(allocator, &entity->anchors);
  darray_child_id_init(allocator, &entity->children);
}

static INLINE void
solparser_entity_release(struct solparser_entity* entity)
{
  ASSERT(entity);
  str_release(&entity->name);
  htable_str2sols_release(&entity->str2anchors);
  htable_str2sols_release(&entity->str2children);
  darray_anchor_id_release(&entity->anchors);
  darray_child_id_release(&entity->children);
}

static INLINE res_T
solparser_entity_copy
  (struct solparser_entity* dst, const struct solparser_entity* src)
{
  res_T res = RES_OK;
  ASSERT(dst && src);
  d3_set(dst->translation, src->translation);
  d3_set(dst->rotation, src->rotation);
  dst->type = src->type;
  dst->data = src->data;
  res = str_copy(&dst->name, &src->name);
  dst->primary = src->primary;
  if(res != RES_OK) return res;
  res = htable_str2sols_copy(&dst->str2anchors, &src->str2anchors);
  if(res != RES_OK) return res;
  res = htable_str2sols_copy(&dst->str2children, &src->str2children);
  if(res != RES_OK) return res;
  res = darray_anchor_id_copy(&dst->anchors, &src->anchors);
  if(res != RES_OK) return res;
  res = darray_child_id_copy(&dst->children, &src->children);
  if(res != RES_OK) return res;
  return RES_OK;
}

static INLINE res_T
solparser_entity_copy_and_release
  (struct solparser_entity* dst, struct solparser_entity* src)
{
  res_T res = RES_OK;
  ASSERT(dst && src);
  d3_set(dst->translation, src->translation);
  d3_set(dst->rotation, src->rotation);
  dst->type = src->type;
  dst->data = src->data;
  res = str_copy_and_release(&dst->name, &src->name);
  dst->primary = src->primary;
  if(res != RES_OK) return res;
  res = htable_str2sols_copy_and_release(&dst->str2anchors, &src->str2anchors);
  if(res != RES_OK) return res;
  res = htable_str2sols_copy_and_release(&dst->str2children, &src->str2children);
  if(res != RES_OK) return res;
  res = darray_anchor_id_copy_and_release(&dst->anchors, &src->anchors);
  if(res != RES_OK) return res;
  res = darray_child_id_copy_and_release(&dst->children, &src->children);
  if(res != RES_OK) return res;
  return RES_OK;
}

static INLINE res_T
solparser_entity_copy_and_clear
  (struct solparser_entity* dst, struct solparser_entity* src)
{
  res_T res = RES_OK;
  ASSERT(dst && src);
  d3_set(dst->translation, src->translation);
  d3_set(dst->rotation, src->rotation);
  dst->type = src->type;
  dst->data = src->data;
  res = str_copy_and_clear(&dst->name, &src->name);
  dst->primary = src->primary;
  if(res != RES_OK) return res;
  res = htable_str2sols_copy_and_clear(&dst->str2anchors, &src->str2anchors);
  if(res != RES_OK) return res;
  res = htable_str2sols_copy_and_clear(&dst->str2children, &src->str2children);
  if(res != RES_OK) return res;
  res = darray_anchor_id_copy_and_clear(&dst->anchors, &src->anchors);
  if(res != RES_OK) return res;
  res = darray_child_id_copy_and_clear(&dst->children, &src->children);
  if(res != RES_OK) return res;
  return RES_OK;
}

static INLINE size_t
solparser_entity_get_anchors_count(const struct solparser_entity* entity)
{
  ASSERT(entity);
  return darray_anchor_id_size_get(&entity->anchors);
}

static INLINE struct solparser_anchor_id
solparser_entity_get_anchor(const struct solparser_entity* entity, const size_t i)
{
  ASSERT(entity && i < solparser_entity_get_anchors_count(entity));
  return darray_anchor_id_cdata_get(&entity->anchors)[i];
}

static INLINE size_t
solparser_entity_get_children_count(const struct solparser_entity* entity)
{
  ASSERT(entity);
  return darray_child_id_size_get(&entity->children);
}

static INLINE struct solparser_entity_id
solparser_entity_get_child(const struct solparser_entity* entity, const size_t i)
{
  ASSERT(entity && i < solparser_entity_get_children_count(entity));
  return darray_child_id_cdata_get(&entity->children)[i];
}

#endif /* SOLPARSER_ENTITY_H */

