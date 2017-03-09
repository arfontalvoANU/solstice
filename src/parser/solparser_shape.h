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

#ifndef SOLPARSER_SHAPE_H
#define SOLPARSER_SHAPE_H

#include "solparser_material.h"

#include <rsys/dynamic_array_double.h>
#include <rsys/str.h>

enum solparser_clip_op {
  SOLPARSER_CLIP_OP_AND,
  SOLPARSER_CLIP_OP_SUB
};

enum solparser_shape_type {
  SOLPARSER_SHAPE_CUBOID,
  SOLPARSER_SHAPE_CYLINDER,
  SOLPARSER_SHAPE_OBJ, /* Imported Alias Wavefront obj */
  SOLPARSER_SHAPE_PARABOL,
  SOLPARSER_SHAPE_PARABOLIC_CYLINDER,
  SOLPARSER_SHAPE_HYPERBOL,
  SOLPARSER_SHAPE_PLANE,
  SOLPARSER_SHAPE_SPHERE,
  SOLPARSER_SHAPE_STL /* Imported STereo Lithography */
};

/*******************************************************************************
 * Clipping polygon
 ******************************************************************************/
struct solparser_polyclip {
  enum solparser_clip_op op;
  struct darray_double vertices;
};

static INLINE void
solparser_polyclip_init
  (struct mem_allocator* allocator,
   struct solparser_polyclip* polyclip)
{
  ASSERT(polyclip);
  darray_double_init(allocator, &polyclip->vertices);
}

static INLINE void
solparser_polyclip_release(struct solparser_polyclip* polyclip)
{
  ASSERT(polyclip);
  darray_double_release(&polyclip->vertices);
}

static INLINE res_T
solparser_polyclip_copy
  (struct solparser_polyclip* dst, const struct solparser_polyclip* src)
{
  ASSERT(dst && src);
  dst->op = src->op;
  return darray_double_copy(&dst->vertices, &src->vertices);
}

static INLINE res_T
solparser_polyclip_copy_and_release
  (struct solparser_polyclip* dst, struct solparser_polyclip* src)
{
  ASSERT(dst && src);
  dst->op = src->op;
  return darray_double_copy_and_release(&dst->vertices, &src->vertices);
}

static INLINE size_t
solparser_polyclip_get_vertices_count
  (const struct solparser_polyclip* polyclip)
{
  size_t n;
  ASSERT(polyclip);
  n = darray_double_size_get(&polyclip->vertices);
  ASSERT((n % 2/*#coords per vertex*/) == 0);
  return n / 2/*#coords per vertex*/;
}

static INLINE void
solparser_polyclip_get_vertex
  (const struct solparser_polyclip* polyclip,
   const size_t ivert,
   double pos[2])
{
  ASSERT(polyclip && ivert < solparser_polyclip_get_vertices_count(polyclip));
  pos[0] = darray_double_cdata_get(&polyclip->vertices)[ivert*2+0];
  pos[1] = darray_double_cdata_get(&polyclip->vertices)[ivert*2+1];
}


/* Declare the array of clipping polygons */
#define DARRAY_NAME polyclip
#define DARRAY_DATA struct solparser_polyclip
#define DARRAY_FUNCTOR_INIT solparser_polyclip_init
#define DARRAY_FUNCTOR_RELEASE solparser_polyclip_release
#define DARRAY_FUNCTOR_COPY solparser_polyclip_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solparser_polyclip_copy_and_release
#include <rsys/dynamic_array.h>

/*******************************************************************************
 * Imported geometry shape
 ******************************************************************************/
struct solparser_shape_imported_geometry {
  struct str filename;
};

static INLINE void
solparser_shape_imported_geometry_init
  (struct mem_allocator* allocator,
   struct solparser_shape_imported_geometry* impgeom)
{
  ASSERT(impgeom);
  str_init(allocator, &impgeom->filename);
}

static INLINE void
solparser_shape_imported_geometry_release
  (struct solparser_shape_imported_geometry* impgeom)
{
  ASSERT(impgeom);
  str_release(&impgeom->filename);
}

static INLINE res_T
solparser_shape_imported_geometry_copy
  (struct solparser_shape_imported_geometry* dst,
   const struct solparser_shape_imported_geometry* src)
{
  ASSERT(dst && src);
  return str_copy(&dst->filename, &src->filename);
}

static INLINE res_T
solparser_shape_imported_geometry_copy_and_release
  (struct solparser_shape_imported_geometry* dst,
   struct solparser_shape_imported_geometry* src)
{
  ASSERT(dst && src);
  return str_copy_and_release(&dst->filename, &src->filename);
}

/*******************************************************************************
 * Paraboloid shape
 ******************************************************************************/
struct solparser_shape_paraboloid {
  double focal;
  struct darray_polyclip polyclips; 
};

static INLINE void
solparser_shape_paraboloid_init
  (struct mem_allocator* allocator,
   struct solparser_shape_paraboloid* paraboloid)
{
  ASSERT(paraboloid);
  darray_polyclip_init(allocator, &paraboloid->polyclips);
}

static INLINE void
solparser_shape_paraboloid_release(struct solparser_shape_paraboloid* paraboloid)
{
  ASSERT(paraboloid);
  darray_polyclip_release(&paraboloid->polyclips);
}

static INLINE res_T
solparser_shape_paraboloid_copy
  (struct solparser_shape_paraboloid* dst,
   const struct solparser_shape_paraboloid* src)
{
  ASSERT(dst && src);
  dst->focal = src->focal;
  return darray_polyclip_copy(&dst->polyclips, &src->polyclips);
}

static INLINE res_T
solparser_shape_paraboloid_copy_and_release
  (struct solparser_shape_paraboloid* dst,
   struct solparser_shape_paraboloid* src)
{
  ASSERT(dst && src);
  dst->focal = src->focal;
  return darray_polyclip_copy_and_release(&dst->polyclips, &src->polyclips);
}

/*******************************************************************************
* Hyperboloid shape
******************************************************************************/
struct hyperboloid_focals {
  double real, image;
};

#define HYPERBOLOID_FOCALS_NULL__ { 0, 0 }
static const struct hyperboloid_focals 
HYPERBOLOID_FOCALS_NULL = HYPERBOLOID_FOCALS_NULL__;

struct solparser_shape_hyperboloid {
  struct hyperboloid_focals focals;
  struct darray_polyclip polyclips;
};

static INLINE void
solparser_shape_hyperboloid_init
  (struct mem_allocator* allocator,
   struct solparser_shape_hyperboloid* hyperboloid)
{
  ASSERT(hyperboloid);
  darray_polyclip_init(allocator, &hyperboloid->polyclips);
}

static INLINE void
solparser_shape_hyperboloid_release(struct solparser_shape_hyperboloid* hyperboloid)
{
  ASSERT(hyperboloid);
  darray_polyclip_release(&hyperboloid->polyclips);
}

static INLINE res_T
solparser_shape_hyperboloid_copy
  (struct solparser_shape_hyperboloid* dst,
   const struct solparser_shape_hyperboloid* src)
{
  ASSERT(dst && src);
  dst->focals = src->focals;
  return darray_polyclip_copy(&dst->polyclips, &src->polyclips);
}

static INLINE res_T
solparser_shape_hyperboloid_copy_and_release
  (struct solparser_shape_hyperboloid* dst,
   struct solparser_shape_hyperboloid* src)
{
  ASSERT(dst && src);
  dst->focals = src->focals;
  return darray_polyclip_copy_and_release(&dst->polyclips, &src->polyclips);
}

/*******************************************************************************
 * Plane shape
 ******************************************************************************/
struct solparser_shape_plane {
  struct darray_polyclip polyclips;
};

static INLINE void
solparser_shape_plane_init
  (struct mem_allocator* allocator,
   struct solparser_shape_plane* plane)
{
  ASSERT(plane);
  darray_polyclip_init(allocator, &plane->polyclips);
}

static INLINE void
solparser_shape_plane_release(struct solparser_shape_plane* plane)
{
  ASSERT(plane);
  darray_polyclip_release(&plane->polyclips);
}

static INLINE res_T
solparser_shape_plane_copy
  (struct solparser_shape_plane* dst,
   const struct solparser_shape_plane* src)
{
  ASSERT(dst && src);
  return darray_polyclip_copy(&dst->polyclips, &src->polyclips);
}

static INLINE res_T
solparser_shape_plane_copy_and_release
  (struct solparser_shape_plane* dst,
   struct solparser_shape_plane* src)
{
  ASSERT(dst && src);
  return darray_polyclip_copy_and_release(&dst->polyclips, &src->polyclips);
}

/*******************************************************************************
 * POD shape data
 ******************************************************************************/
struct solparser_shape_cuboid {
  double size[3]; /* Size along the X, Y and Z dimension */
};

struct solparser_shape_cylinder {
  double height;
  double radius;
  long nslices;
};

struct solparser_shape_sphere {
  double radius;
  long nslices;
};

struct solparser_shape_cuboid_id { size_t i; };
struct solparser_shape_cylinder_id { size_t i; };
struct solparser_shape_imported_geometry_id { size_t i; };
struct solparser_shape_paraboloid_id { size_t i; };
struct solparser_shape_hyperboloid_id { size_t i; };
struct solparser_shape_plane_id { size_t i; };
struct solparser_shape_sphere_id { size_t i; };

struct solparser_shape {
  enum solparser_shape_type type;
  union {
    struct solparser_shape_cuboid_id cuboid;
    struct solparser_shape_cylinder_id cylinder;
    struct solparser_shape_imported_geometry_id obj;
    struct solparser_shape_paraboloid_id parabol;
    struct solparser_shape_paraboloid_id parabolic_cylinder;
    struct solparser_shape_hyperboloid_id hyperbol;
    struct solparser_shape_plane_id plane;
    struct solparser_shape_sphere_id sphere;
    struct solparser_shape_imported_geometry_id stl;
  } data;
};

struct solparser_shape_id { size_t i; };

#endif /* SOLPARSER_SHAPE_H */

