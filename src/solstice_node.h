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

#ifndef SOLSTICE_NODE_H
#define SOLSTICE_NODE_H

#include "solstice_shape.h"

#include <rsys/double3.h>
#include <rsys/dynamic_array.h>
#include <rsys/list.h>

struct solstice_node_id { size_t i; };

#define DARRAY_NAME geometry
#define DARRAY_DATA struct solstice_object_id
#include <rsys/dynamic_array.h>

#define DARRAY_NAME child
#define DARRAY_DATA struct solstice_node_id
#include <rsys/dynamic_array.h>

struct solstice_node {
  double rotation[3];
  double translation[3];

  /* TODO pivot */
  struct darray_geometry geometries; /* List of geometries */
  struct darray_child children; /* List of children nodes */
};

static INLINE void
solstice_node_init(struct mem_allocator* allocator, struct solstice_node* node)
{
  ASSERT(node);
  d3_splat(node->rotation, 0);
  d3_splat(node->translation, 0);
  darray_geometry_init(allocator, &node->geometries);
  darray_child_init(allocator, &node->children);
}

static INLINE void
solstice_node_release(struct solstice_node* node)
{
  ASSERT(node);
  darray_geometry_release(&node->geometries);
  darray_child_release(&node->children);
}

static INLINE res_T
solstice_node_copy(struct solstice_node* dst, const struct solstice_node* src)
{
  res_T res = RES_OK;
  ASSERT(dst && src);
  d3_set(dst->translation, src->translation);
  d3_set(dst->rotation, src->rotation);
  res = darray_geometry_copy(&dst->geometries, &src->geometries);
  if(res != RES_OK) return res;
  return darray_child_copy(&dst->children, &src->children);
}

static INLINE res_T
solstice_node_copy_and_release
  (struct solstice_node* dst, struct solstice_node* src)
{
  res_T res = RES_OK;
  ASSERT(dst && src);
  d3_set(dst->translation, src->translation);
  d3_set(dst->rotation, src->rotation);
  res = darray_geometry_copy_and_release(&dst->geometries, &src->geometries);
  if(res != RES_OK) return res;
  return darray_child_copy_and_release(&dst->children, &src->children);
}

#endif /* SOLSTICE_NODE_H */

