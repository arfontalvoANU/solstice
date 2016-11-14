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

#ifndef SOLSTICE_SHAPE_H
#define SOLSTICE_SHAPE_H

#include <rsys/dynamic_array_double.h>
#include <rsys/str.h>

enum solstice_clip_op {
  SOLSTICE_CLIP_OP_AND,
  SOLSTICE_CLIP_OP_SUB
};

enum solstice_shape_type {
  SOLSTICE_SHAPE_CUBOID,
  SOLSTICE_SHAPE_CYLINDER,
  SOLSTICE_SHAPE_OBJ, /* Imported Alias Wavefront obj */
  SOLSTICE_SHAPE_PARABOL,
  SOLSTICE_SHAPE_PARABOLIC_CYLINDER,
  SOLSTICE_SHAPE_PLANE,
  SOLSTICE_SHAPE_SPHERE,
  SOLSTICE_SHAPE_STL /* Imported STereo Lithography */
};

struct solstice_polyclip {
  enum solstice_clip_op op;
  struct darray_double vertices;
};

static INLINE void
solstice_polyclip_init
  (struct mem_allocator* alloc,
   struct solstice_polyclip* polyclip)
{
  ASSERT(polyclip);
  darray_double_init(alloc, &polyclip->vertices);
}

static INLINE void
solstice_polyclip_release(struct solstice_polyclip* polyclip)
{
  ASSERT(polyclip);
  darray_double_release(&polyclip->vertices);
}

static INLINE res_T
solstice_polyclip_copy
  (struct solstice_polyclip* dst, const struct solstice_polyclip* src)
{
  ASSERT(dst && src);
  dst->op = src->op;
  return darray_double_copy(&dst->vertices, &src->vertices);
}

static INLINE res_T
solstice_polyclip_copy_and_release
  (struct solstice_polyclip* dst, struct solstice_polyclip* src)
{
  ASSERT(dst && src);
  dst->op = src->op;
  return darray_double_copy_and_release(&dst->vertices, &src->vertices);
}

/* Declare the array of clipping polygons */
#define DARRAY_NAME polyclip
#define DARRAY_DATA struct solstice_polyclip
#define DARRAY_FUNCTOR_INIT solstice_polyclip_init
#define DARRAY_FUNCTOR_RELEASE solstice_polyclip_release
#define DARRAY_FUNCTOR_COPY solstice_polyclip_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solstice_polyclip_copy_and_release
#include <rsys/dynamic_array.h>

struct solstice_shape_cuboid {
  double size[3]; /* Size along the X, Y and Z dimension */
};

struct solstice_shape_cylinder {
  double height;
  double radius;
  size_t nslices;
};

struct solstice_shape_imported_geometry {
  struct str filename;
};

struct solstice_shape_parabol {
  double focal;
  struct darray_polyclip* polyclips;
};

struct solstice_shape_parabolic_cylinder {
  double focal;
  struct darray_polyclip* polyclips;
};

struct solstice_shape_plane {
  struct darray_polyclip* polyclips;
};

struct solstice_shape_sphere {
  double radius;
  size_t nslices;
};

struct solstice_shape {
  enum solstice_shape_type type;
  union {
    struct solstice_shape_cuboid cuboid;
    struct solstice_shape_cylinder cylinder;
    struct solstice_shape_imported_geometry imported_geom;
    struct solstice_shape_parabol parabol;
    struct solstice_shape_parabolic_cylinder parabolic_cylinder;
    struct solstice_shape_plane plane;
    struct solstice_shape_sphere sphere;
  } data;
};

#endif /* SOLSTICE_SHAPE_H */

