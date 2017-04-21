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

#ifndef SOLPARSER_H
#define SOLPARSER_H

#include "solparser_entity.h"
#include "solparser_spectrum.h"
#include <rsys/rsys.h>
#include <stddef.h>

struct mem_allocator;
struct solparser;

#define SOLPARSER_ID_IS_VALID(Id) ((Id).i != SIZE_MAX)

struct solparser_entity_iterator {
  struct htable_str2sols_iterator it__; /* Internal data */
};

struct solparser_material_iterator {
  /* Internal data */
  const struct solparser_material* mtls__;
  size_t imtl__;
};

struct solparser_geometry_iterator {
  /* Internal data */
  const struct solparser_geometry* geoms__;
  size_t igeom__;
};

/*******************************************************************************
 * Solstice parser API.
 ******************************************************************************/
extern LOCAL_SYM res_T
solparser_create
  (struct mem_allocator* allocator, /* May be NULL <=> use default allocator */
   struct solparser** parser);

extern LOCAL_SYM void
solparser_ref_get
  (struct solparser* parser);

extern LOCAL_SYM void
solparser_ref_put
  (struct solparser* parser);

extern LOCAL_SYM res_T
solparser_setup
  (struct solparser* parser,
   const char* stream_name, /* May be NULL */
   FILE* stream);

/* Return RES_BAD_OP if there is no more YAML document to parse */
extern LOCAL_SYM res_T
solparser_load
  (struct solparser* parser);

/* Return NULL if no entity is found */
extern LOCAL_SYM const struct solparser_anchor*
solparser_find_anchor
  (struct solparser* parser,
   const char* anchor_name);

/* Return NULL if no entity is found */
extern LOCAL_SYM const struct solparser_entity*
solparser_find_entity
  (struct solparser* parser,
   const char* entity_name);

extern LOCAL_SYM const struct solparser_anchor*
solparser_get_anchor
  (const struct solparser* parser,
   const struct solparser_anchor_id anchor);

extern LOCAL_SYM const struct solparser_entity*
solparser_get_entity
  (const struct solparser* parser,
   const struct solparser_entity_id entity);

extern LOCAL_SYM const struct solparser_image*
solparser_get_image
  (const struct solparser* parser,
   const struct solparser_image_id image);

extern LOCAL_SYM const struct solparser_geometry*
solparser_get_geometry
  (const struct solparser* parser,
   const struct solparser_geometry_id geom);

extern LOCAL_SYM const struct solparser_material*
solparser_get_material
  (const struct solparser* parser,
   const struct solparser_material_id mtl);

extern LOCAL_SYM const struct solparser_material_double_sided*
solparser_get_material_double_sided
  (const struct solparser* parser,
   const struct solparser_material_double_sided_id mtl2);

extern LOCAL_SYM const struct solparser_material_dielectric*
solparser_get_material_dielectric
  (const struct solparser* parser,
   const struct solparser_material_dielectric_id dielectric);

extern LOCAL_SYM const struct solparser_material_matte*
solparser_get_material_matte
  (const struct solparser* parser,
   const struct solparser_material_matte_id matte);

extern LOCAL_SYM const struct solparser_material_mirror*
solparser_get_material_mirror
  (const struct solparser* parser,
   const struct solparser_material_mirror_id mirror);

extern LOCAL_SYM const struct solparser_material_thin_dielectric*
solparser_get_material_thin_dielectric
  (const struct solparser* parser,
   const struct solparser_material_thin_dielectric_id thin_dielectric);

extern LOCAL_SYM const struct solparser_medium*
solparser_get_medium
  (const struct solparser* parser,
   const struct solparser_medium_id medium);

extern LOCAL_SYM const struct solparser_object*
solparser_get_object
  (const struct solparser* parser,
   const struct solparser_object_id obj);

extern LOCAL_SYM const struct solparser_shape*
solparser_get_shape
  (const struct solparser* parser,
   const struct solparser_shape_id shape);

extern LOCAL_SYM const struct solparser_shape_cuboid*
solparser_get_shape_cuboid
  (const struct solparser* parser,
   const struct solparser_shape_cuboid_id cuboid);

extern LOCAL_SYM const struct solparser_shape_cylinder*
solparser_get_shape_cylinder
  (const struct solparser* parser,
   const struct solparser_shape_cylinder_id cylinder);

extern LOCAL_SYM const struct solparser_shape_imported_geometry*
solparser_get_shape_obj
  (const struct solparser* parser,
   const struct solparser_shape_imported_geometry_id impgeom);

extern LOCAL_SYM const struct solparser_shape_paraboloid*
solparser_get_shape_parabol
  (const struct solparser* parser,
   const struct solparser_shape_paraboloid_id paraboloid);

extern LOCAL_SYM const struct solparser_shape_paraboloid*
solparser_get_shape_parabolic_cylinder
  (const struct solparser* parser,
   const struct solparser_shape_paraboloid_id paraboloid);

extern LOCAL_SYM const struct solparser_shape_hyperboloid*
solparser_get_shape_hyperbol
  (const struct solparser* parser,
   const struct solparser_shape_hyperboloid_id hyperboloid);

extern LOCAL_SYM const struct solparser_shape_hemisphere*
solparser_get_shape_hemisphere
  (const struct solparser* parser,
   const struct solparser_shape_hemisphere_id hemisphere);

extern LOCAL_SYM const struct solparser_shape_plane*
solparser_get_shape_plane
  (const struct solparser* parser,
   const struct solparser_shape_plane_id plane);

extern LOCAL_SYM const struct solparser_shape_sphere*
solparser_get_shape_sphere
  (const struct solparser* parser,
   const struct solparser_shape_sphere_id sphere);

extern LOCAL_SYM const struct solparser_shape_imported_geometry*
solparser_get_shape_stl
  (const struct solparser* parser,
   const struct solparser_shape_imported_geometry_id impgeom);

extern LOCAL_SYM const struct solparser_sun*
solparser_get_sun
  (const struct solparser* parser);

extern LOCAL_SYM const struct solparser_x_pivot*
solparser_get_x_pivot
  (const struct solparser* parser,
   const struct solparser_pivot_id x_pivot);

extern LOCAL_SYM const struct solparser_zx_pivot*
solparser_get_zx_pivot
  (const struct solparser* parser,
   const struct solparser_pivot_id zx_pivot);

extern LOCAL_SYM const struct solparser_spectrum*
solparser_get_spectrum
  (const struct solparser* parser,
   const struct solparser_spectrum_id spectrum);

/*******************************************************************************
 * Entity interator
 ******************************************************************************/
extern LOCAL_SYM void
solparser_entity_iterator_begin
  (struct solparser* parser,
   struct solparser_entity_iterator* it);

extern LOCAL_SYM void
solparser_entity_iterator_end
  (struct solparser* parser,
   struct solparser_entity_iterator* it);

static FINLINE void
solparser_entity_iterator_next(struct solparser_entity_iterator* it)
{
  ASSERT(it);
  htable_str2sols_iterator_next(&it->it__);
}

static FINLINE int
solparser_entity_iterator_eq
  (const struct solparser_entity_iterator* a,
   const struct solparser_entity_iterator* b)
{
  ASSERT(a && b);
  return htable_str2sols_iterator_eq(&a->it__, &b->it__);
}

static FINLINE struct solparser_entity_id
solparser_entity_iterator_get(struct solparser_entity_iterator* it)
{
  struct solparser_entity_id id;
  ASSERT(it);
  id.i = *htable_str2sols_iterator_data_get(&it->it__);
  return id;
}

/*******************************************************************************
 * Material iterator
 ******************************************************************************/
extern LOCAL_SYM void
solparser_material_iterator_begin
  (struct solparser* parser,
   struct solparser_material_iterator* it);

extern LOCAL_SYM void
solparser_material_iterator_end
  (struct solparser* parser,
   struct solparser_material_iterator* it);

static FINLINE void
solparser_material_iterator_next(struct solparser_material_iterator* it)
{
  ASSERT(it);
  ++it->imtl__;
}

static FINLINE int
solparser_material_iterator_eq
  (const struct solparser_material_iterator* a,
   const struct solparser_material_iterator* b)
{
  ASSERT(a && b);
  return a->mtls__ == b->mtls__ && a->imtl__ == b->imtl__;
}

static FINLINE struct solparser_material_id
solparser_material_iterator_get(struct solparser_material_iterator* it)
{
  struct solparser_material_id id;
  ASSERT(it);
  id.i = it->imtl__;
  return id;
}

/*******************************************************************************
 * Geometry iterator
 ******************************************************************************/
extern LOCAL_SYM void
solparser_geometry_iterator_begin
  (struct solparser* parser, struct solparser_geometry_iterator* it);

extern LOCAL_SYM void
solparser_geometry_iterator_end
  (struct solparser* parser, struct solparser_geometry_iterator* it);

static FINLINE void
solparser_geometry_iterator_next(struct solparser_geometry_iterator* it)
{
  ASSERT(it);
  ++it->igeom__;
}

static FINLINE int
solparser_geometry_iterator_eq
  (const struct solparser_geometry_iterator* a,
   const struct solparser_geometry_iterator* b)
{
  ASSERT(a && b);
  return a->geoms__ == b->geoms__ && a->igeom__ == b->igeom__;
}

static FINLINE struct solparser_geometry_id
solparser_geometry_iterator_get(struct solparser_geometry_iterator* it)
{
  struct solparser_geometry_id id;
  ASSERT(it);
  id.i = it->igeom__;
  return id;
}

#endif /* SOLPARSER_H */

