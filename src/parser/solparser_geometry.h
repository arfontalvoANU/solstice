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

#ifndef SOLPARSER_GEOMETRY_H
#define SOLPARSER_GEOMETRY_H

#include "solparser_material.h"
#include "solparser_shape.h"

#include <rsys/dynamic_array.h>

struct solparser_object {
  struct solparser_material_double_sided_id mtl2;
  struct solparser_shape_id shape;
  double rotation[3]; /* In degrees */
  double translation[3];
};

struct solparser_object_id { size_t i; };

#define DARRAY_NAME object_id
#define DARRAY_DATA struct solparser_object_id
#include <rsys/dynamic_array.h>

struct solparser_geometry {
  /* Internal data. Should not be acceded directly. */
  struct darray_object_id objects;
};

struct solparser_geometry_id { size_t i; };

static INLINE void
solparser_geometry_init
  (struct mem_allocator* allocator, struct solparser_geometry* geom)
{
  ASSERT(geom);
  darray_object_id_init(allocator, &geom->objects);
}

static INLINE void
solparser_geometry_release(struct solparser_geometry* geom)
{
  ASSERT(geom);
  darray_object_id_release(&geom->objects);
}

static INLINE res_T
solparser_geometry_copy
  (struct solparser_geometry* dst, const struct solparser_geometry* src)
{
  ASSERT(dst && src);
  return darray_object_id_copy(&dst->objects, &src->objects);
}

static INLINE res_T
solparser_geometry_copy_and_release
  (struct solparser_geometry* dst, struct solparser_geometry* src)
{
  ASSERT(dst && src);
  return darray_object_id_copy_and_release(&dst->objects, &src->objects);
}

static FINLINE size_t
solparser_geometry_get_objects_count(const struct solparser_geometry* geom)
{
  ASSERT(geom);
  return darray_object_id_size_get(&geom->objects);
}

static FINLINE struct solparser_object_id
solparser_geometry_get_object
  (const struct solparser_geometry* geom, const size_t i)
{
  ASSERT(geom && i < solparser_geometry_get_objects_count(geom));
  return darray_object_id_cdata_get(&geom->objects)[i];
}

#endif /* SOLPARSER_GEOMETRY_H */
