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

#ifndef SOLSTICE_GEOMETRY_H
#define SOLSTICE_GEOMETRY_H

#include "solstice_material.h"
#include "solstice_shape.h"

#include <rsys/dynamic_array.h>

struct solstice_object {
  struct solstice_material_double_sided_id mtl2;
  struct solstice_shape_id shape;
  double rotation[3];
  double translation[3];
};

struct solstice_object_id { size_t i; };

#define DARRAY_NAME object_id
#define DARRAY_DATA struct solstice_object_id
#include <rsys/dynamic_array.h>

struct solstice_geometry {
  /* Internal data. Should not be acceded directly. */
  struct darray_object_id objects;
};

struct solstice_geometry_id { size_t i; };

static INLINE void
solstice_geometry_init
  (struct mem_allocator* allocator, struct solstice_geometry* geom)
{
  ASSERT(geom);
  darray_object_id_init(allocator, &geom->objects);
}

static INLINE void
solstice_geometry_release(struct solstice_geometry* geom)
{
  ASSERT(geom);
  darray_object_id_release(&geom->objects);
}

static INLINE res_T
solstice_geometry_copy
  (struct solstice_geometry* dst, const struct solstice_geometry* src)
{
  ASSERT(dst && src);
  return darray_object_id_copy(&dst->objects, &src->objects);
}

static INLINE res_T
solstice_geometry_copy_and_release
  (struct solstice_geometry* dst, struct solstice_geometry* src)
{
  ASSERT(dst && src);
  return darray_object_id_copy_and_release(&dst->objects, &src->objects);
}

static FINLINE size_t
solstice_geometry_get_objects_count(const struct solstice_geometry* geom)
{
  ASSERT(geom);
  return darray_object_id_size_get(&geom->objects);
}

static FINLINE struct solstice_object_id
solstice_geometry_get_object
  (const struct solstice_geometry* geom, const size_t i)
{
  ASSERT(geom && i < solstice_geometry_get_objects_count(geom));
  return darray_object_id_cdata_get(&geom->objects)[i];
}

#endif /* SOLSTICE_GEOMETRY_H */
