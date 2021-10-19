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

#ifndef SOLPARSER_SUN_H
#define SOLPARSER_SUN_H

#include "solparser_spectrum.h"
#include <rsys/dynamic_array.h>

enum solparser_sun_radang_distrib_type { /* Radial Angular distribution */
  SOLPARSER_SUN_RADANG_DISTRIB_BUIE,
  SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL,
  SOLPARSER_SUN_RADANG_DISTRIB_PILLBOX,
  SOLPARSER_SUN_RADANG_DISTRIB_GAUSSIAN
};

struct solparser_sun_buie { double csr; };
struct solparser_sun_pillbox { double half_angle; };
struct solparser_sun_gaussian { double std_dev; };

struct solparser_sun {
  double dni; /* In ]0, INF) */
  struct solparser_spectrum_id spectrum;
  enum solparser_sun_radang_distrib_type radang_distrib_type;
  union {
    struct solparser_sun_buie buie;
    struct solparser_sun_pillbox pillbox;
    struct solparser_sun_gaussian gaussian;
  } radang_distrib;
};

static INLINE void
solparser_sun_init(struct mem_allocator* allocator, struct solparser_sun* sun)
{
  ASSERT(sun);
  (void)allocator;
  sun->dni = 1.0;
  sun->radang_distrib_type = SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL;
  sun->spectrum.i = SIZE_MAX;
}

static INLINE void
solparser_sun_release(struct solparser_sun* sun)
{
  ASSERT(sun);
  (void)sun;
  /* Do nothing */
}

static INLINE void
solparser_sun_clear(struct solparser_sun* sun)
{
  ASSERT(sun);
  sun->spectrum.i = SIZE_MAX;
}

#endif /* SOLPARSER_SUN_H */

