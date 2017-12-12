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

#ifndef SOLPARSER_MATERIAL_H
#define SOLPARSER_MATERIAL_H

#include "solparser_image.h"
#include "solparser_medium.h"
#include <stddef.h>

enum solparser_material_type {
  SOLPARSER_MATERIAL_DIELECTRIC,
  SOLPARSER_MATERIAL_MATTE,
  SOLPARSER_MATERIAL_MIRROR,
  SOLPARSER_MATERIAL_THIN_DIELECTRIC,
  SOLPARSER_MATERIAL_VIRTUAL
};

enum solparser_microfacet_distribution {
  SOLPARSER_MICROFACET_BECKMANN,
  SOLPARSER_MICROFACET_PILLBOX
};

struct solparser_material_dielectric {
  struct solparser_medium_id medium_i; /* Medium the material "looks at" */
  struct solparser_medium_id medium_t; /* Opposite medium */
  struct solparser_image_id normal_map;
};
struct solparser_material_dielectric_id { size_t i; };

static INLINE void
solparser_material_dielectric_init
  (struct mem_allocator* allocator,
   struct solparser_material_dielectric* dielectric)
{
  ASSERT(dielectric);
  (void)allocator;
  dielectric->normal_map.i = SIZE_MAX;
}

struct solparser_material_matte {
  struct solparser_mtl_data reflectivity; /* In [0, 1] */
  struct solparser_image_id normal_map;
};

struct solparser_material_matte_id { size_t i; };

static INLINE void
solparser_material_matte_init
  (struct mem_allocator* allocator,
   struct solparser_material_matte* matte)
{
  ASSERT(matte);
  (void)allocator;
  matte->normal_map.i = SIZE_MAX;
}

struct solparser_material_mirror {
  struct solparser_mtl_data roughness; /* In [0, 1] */
  struct solparser_mtl_data reflectivity; /* In [0, 1] */
  enum solparser_microfacet_distribution ufacet_distrib;
  struct solparser_image_id normal_map;
};

struct solparser_material_mirror_id { size_t i; };

static INLINE void
solparser_material_mirror_init
  (struct mem_allocator* allocator,
   struct solparser_material_mirror* mirror)
{
  ASSERT(mirror);
  (void)allocator;
  mirror->roughness.type = SOLPARSER_MTL_DATA_REAL;
  mirror->roughness.value.real = 0;
  mirror->ufacet_distrib = SOLPARSER_MICROFACET_BECKMANN;
  mirror->normal_map.i = SIZE_MAX;
}

struct solparser_material_thin_dielectric {
  struct solparser_medium_id medium_i; /* Outside medium */
  struct solparser_medium_id medium_t; /* Medium of the slab */
  struct solparser_image_id normal_map;
  double thickness;
};

struct solparser_material_thin_dielectric_id { size_t i; };

static INLINE void
solparser_material_thin_dielectric_init
  (struct mem_allocator* allocator,
   struct solparser_material_thin_dielectric* thin)
{
  ASSERT(thin);
  (void)allocator;
  thin->normal_map.i = SIZE_MAX;
}

struct solparser_material {
  enum solparser_material_type type;
  union {
    struct solparser_material_dielectric_id dielectric;
    struct solparser_material_matte_id matte;
    struct solparser_material_mirror_id mirror;
    struct solparser_material_thin_dielectric_id thin_dielectric;
  } data;
};

struct solparser_material_id { size_t i; };

struct solparser_material_double_sided {
  struct solparser_material_id front;
  struct solparser_material_id back;
};

struct solparser_material_double_sided_id { size_t i; };

#endif /* SOLPARSER_MATERIAL_H */
