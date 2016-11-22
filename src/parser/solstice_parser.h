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

#ifndef SOLSTICE_PARSER_H
#define SOLSTICE_PARSER_H

#include "solstice_entity.h"
#include <rsys/rsys.h>

struct mem_allocator;
struct solstice_parser;

struct solstice_entity_iterator {
  struct htable_str2sols_iterator it__; /* Internal data */
};

struct solstice_material_iterator {
  /* Internal data */
  const struct solstice_material* mtls__;
  size_t imtl__;
};

struct solstice_geometry_iterator {
  /* Internal data */
  const struct solstice_geometry* geoms__;
  size_t igeom__;
};

/*******************************************************************************
 * Solstice parser API.
 ******************************************************************************/
extern LOCAL_SYM res_T
solstice_parser_create
  (struct mem_allocator* allocator, /* May be NULL <=> use default allocator */
   struct solstice_parser** parser);

extern LOCAL_SYM void
solstice_parser_ref_get
  (struct solstice_parser* parser);

extern LOCAL_SYM void
solstice_parser_ref_put
  (struct solstice_parser* parser);

extern LOCAL_SYM res_T
solstice_parser_setup
  (struct solstice_parser* parser,
   const char* stream_name, /* May be NULL */
   FILE* stream);

/* Return RES_BAD_OP if there is no more YAML document to parse */
extern LOCAL_SYM res_T
solstice_parser_load
  (struct solstice_parser* parser);

/* Return NULL if the no entity is found */
extern LOCAL_SYM const struct solstice_entity*
solstice_parser_find_entity
  (struct solstice_parser* parser,
   const char* entity_name);

extern LOCAL_SYM const struct solstice_entity*
solstice_parser_get_entity
  (const struct solstice_parser* parser,
   const struct solstice_entity_id entity);

extern LOCAL_SYM const struct solstice_geometry*
solstice_parser_get_geometry
  (const struct solstice_parser* parser,
   const struct solstice_geometry_id geom);

extern LOCAL_SYM const struct solstice_material*
solstice_parser_get_material
  (const struct solstice_parser* parser,
   const struct solstice_material_id mtl);

extern LOCAL_SYM const struct solstice_material_double_sided*
solstice_parser_get_material_double_sided
  (const struct solstice_parser* parser,
   const struct solstice_material_double_sided_id mtl2);

extern LOCAL_SYM const struct solstice_material_matte*
solstice_parser_get_material_matte
  (const struct solstice_parser* parser,
   const struct solstice_material_matte_id matte);

extern LOCAL_SYM const struct solstice_material_mirror*
solstice_parser_get_material_mirror
  (const struct solstice_parser* parser,
   const struct solstice_material_mirror_id mirror);

extern LOCAL_SYM const struct solstice_object*
solstice_parser_get_object
  (const struct solstice_parser* parser,
   const struct solstice_object_id obj);

extern LOCAL_SYM const struct solstice_shape*
solstice_parser_get_shape
  (const struct solstice_parser* parser,
   const struct solstice_shape_id shape);

extern LOCAL_SYM const struct solstice_shape_cuboid*
solstice_parser_get_shape_cuboid
  (const struct solstice_parser* parser,
   const struct solstice_shape_cuboid_id cuboid);

extern LOCAL_SYM const struct solstice_shape_cylinder*
solstice_parser_get_shape_cylinder
  (const struct solstice_parser* parser,
   const struct solstice_shape_cylinder_id cylinder);

extern LOCAL_SYM const struct solstice_shape_imported_geometry*
solstice_parser_get_shape_obj
  (const struct solstice_parser* parser,
   const struct solstice_shape_imported_geometry_id impgeom);

extern LOCAL_SYM const struct solstice_shape_paraboloid*
solstice_parser_get_shape_parabol
  (const struct solstice_parser* parser,
   const struct solstice_shape_paraboloid_id paraboloid);

extern LOCAL_SYM const struct solstice_shape_paraboloid*
solstice_parser_get_shape_parabolic_cylinder
  (const struct solstice_parser* parser,
   const struct solstice_shape_paraboloid_id paraboloid);

extern LOCAL_SYM const struct solstice_shape_plane*
solstice_parser_get_shape_plane
  (const struct solstice_parser* parser,
   const struct solstice_shape_plane_id plane);

extern LOCAL_SYM const struct solstice_shape_sphere*
solstice_parser_get_shape_sphere
  (const struct solstice_parser* parser,
   const struct solstice_shape_sphere_id sphere);

extern LOCAL_SYM const struct solstice_shape_imported_geometry*
solstice_parser_get_shape_stl
  (const struct solstice_parser* parser,
   const struct solstice_shape_imported_geometry_id impgeom);

extern LOCAL_SYM const struct solstice_sun*
solstice_parser_get_sun
  (const struct solstice_parser* parser);

/*******************************************************************************
 * Entity interator
 ******************************************************************************/
extern LOCAL_SYM void
solstice_parser_entity_iterator_begin
  (struct solstice_parser* parser,
   struct solstice_entity_iterator* it);

extern LOCAL_SYM void
solstice_parser_entity_iterator_end
  (struct solstice_parser* parser,
   struct solstice_entity_iterator* it);

static FINLINE void
solstice_entity_iterator_next(struct solstice_entity_iterator* it)
{
  ASSERT(it);
  htable_str2sols_iterator_next(&it->it__);
}

static FINLINE int
solstice_entity_iterator_eq
  (const struct solstice_entity_iterator* a,
   const struct solstice_entity_iterator* b)
{
  ASSERT(a && b);
  return htable_str2sols_iterator_eq(&a->it__, &b->it__);
}

static FINLINE struct solstice_entity_id
solstice_entity_iterator_get(struct solstice_entity_iterator* it)
{
  struct solstice_entity_id id;
  ASSERT(it);
  id.i = *htable_str2sols_iterator_data_get(&it->it__);
  return id;
}

/*******************************************************************************
 * Material iterator
 ******************************************************************************/
extern LOCAL_SYM void
solstice_parser_material_iterator_begin
  (struct solstice_parser* parser,
   struct solstice_material_iterator* it);

extern LOCAL_SYM void
solstice_parser_material_iterator_end
  (struct solstice_parser* parser,
   struct solstice_material_iterator* it);

static FINLINE void
solstice_material_iterator_next(struct solstice_material_iterator* it)
{
  ASSERT(it);
  ++it->imtl__;
}

static FINLINE int
solstice_material_iterator_eq
  (const struct solstice_material_iterator* a,
   const struct solstice_material_iterator* b)
{
  ASSERT(a && b);
  return a->mtls__ == b->mtls__ && a->imtl__ == b->imtl__;
}

static FINLINE struct solstice_material_id
solstice_material_iterator_get(struct solstice_material_iterator* it)
{
  struct solstice_material_id id;
  ASSERT(it);
  id.i = it->imtl__;
  return id;
}

/*******************************************************************************
 * Geometry iterator
 ******************************************************************************/
extern LOCAL_SYM void
solstice_parser_geometry_iterator_begin
  (struct solstice_parser* parser,
   struct solstice_geometry_iterator* it);

extern LOCAL_SYM void
solstice_parser_geometry_iterator_end
  (struct solstice_parser* parser,
   struct solstice_geometry_iterator* it);

static FINLINE void
solstice_geometry_iterator_next(struct solstice_geometry_iterator* it)
{
  ASSERT(it);
  ++it->igeom__;
}

static FINLINE int
solstice_geometry_iterator_eq
  (const struct solstice_geometry_iterator* a,
   const struct solstice_geometry_iterator* b)
{
  ASSERT(a && b);
  return a->geoms__ == b->geoms__ && a->igeom__ == b->igeom__;
}

static FINLINE struct solstice_geometry_id
solstice_geometry_iterator_get(struct solstice_geometry_iterator* it)
{
  struct solstice_geometry_id id;
  ASSERT(it);
  id.i = it->igeom__;
  return id;
}

#endif /* SOLSTICE_PARSER_H */

