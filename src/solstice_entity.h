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

#ifndef SOLSTICE_ENTITY_H
#define SOLSTICE_ENTITY_H

#include "solstice_shape.h"
#include "solstice_geometry.h"

#include <rsys/double3.h>
#include <rsys/dynamic_array.h>
#include <rsys/hash_table.h>
#include <rsys/list.h>
#include <rsys/str.h>

struct solstice_entity_id { size_t i; };

#define DARRAY_NAME child
#define DARRAY_DATA struct solstice_entity_id
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

struct solstice_entity {
  double rotation[3];
  double translation[3];

  struct str name;
  struct solstice_geometry_id geometry;

  /* Internal data. Should not be acceded directly. */
  struct htable_str2sols str2children;
  struct darray_child children; /* List of children nodes */
};

static INLINE void
solstice_entity_init
  (struct mem_allocator* allocator, struct solstice_entity* entity)
{
  ASSERT(entity);
  d3_splat(entity->rotation, 0);
  d3_splat(entity->translation, 0);
  entity->geometry.i = SIZE_MAX;
  str_init(allocator, &entity->name);
  htable_str2sols_init(allocator, &entity->str2children);
  darray_child_init(allocator, &entity->children);
}

static INLINE void
solstice_entity_release(struct solstice_entity* entity)
{
  ASSERT(entity);
  str_release(&entity->name);
  htable_str2sols_release(&entity->str2children);
  darray_child_release(&entity->children);
}

static INLINE res_T
solstice_entity_copy
  (struct solstice_entity* dst, const struct solstice_entity* src)
{
  res_T res = RES_OK;
  ASSERT(dst && src);
  d3_set(dst->translation, src->translation);
  d3_set(dst->rotation, src->rotation);
  dst->geometry = src->geometry;
  res = str_copy(&dst->name, &src->name);
  if(res != RES_OK) return res;
  res = htable_str2sols_copy(&dst->str2children, &src->str2children);
  if(res != RES_OK) return res;
  res = darray_child_copy(&dst->children, &src->children);
  if(res != RES_OK) return res;
  return RES_OK;
}

static INLINE res_T
solstice_entity_copy_and_release
  (struct solstice_entity* dst, struct solstice_entity* src)
{
  res_T res = RES_OK;
  ASSERT(dst && src);
  d3_set(dst->translation, src->translation);
  d3_set(dst->rotation, src->rotation);
  dst->geometry = src->geometry;
  res = str_copy_and_release(&dst->name, &src->name);
  if(res != RES_OK) return res;
  res = htable_str2sols_copy_and_release(&dst->str2children, &src->str2children);
  if(res != RES_OK) return res;
  res = darray_child_copy_and_release(&dst->children, &src->children);
  if(res != RES_OK) return res;
  return RES_OK;
}

static INLINE size_t
solstice_entity_get_children_count(const struct solstice_entity* entity)
{
  ASSERT(entity);
  return darray_child_size_get(&entity->children);
}

static INLINE struct solstice_entity_id
solstice_entity_get_child(const struct solstice_entity* entity, const size_t i)
{
  ASSERT(entity && i < solstice_entity_get_children_count(entity));
  return darray_child_cdata_get(&entity->children)[i];
}

#endif /* SOLSTICE_ENTITY_H */

