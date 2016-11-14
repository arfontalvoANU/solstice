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

#ifndef SOLSTICE_SUN_H
#define SOLSTICE_SUN_H

#include <rsys/dynamic_array.h>

enum solstice_sun_radang_distrib_type { /* Radial Angular distribution */
  SOLSTICE_SUN_RADANG_DISTRIB_BUIE,
  SOLSTICE_SUN_RADANG_DISTRIB_DIRECTIONAL,
  SOLSTICE_SUN_RADANG_DISTRIB_PILLBOX
};

struct solstice_spectrum_data {
  double wavelength;
  double data;
};

#define DARRAY_NAME spectrum_data
#define DARRAY_DATA struct solstice_spectrum_data
#include <rsys/dynamic_array.h>

struct solstice_sun_buie { double csr; };
struct solstice_sun_pillbox { double aperture; };

struct solstice_sun {
  double dni; /* In ]0, INF) */
  struct darray_spectrum_data spectrum;
  enum solstice_sun_radang_distrib_type radang_distrib_type;
  union {
    struct solstice_sun_buie buie;
    struct solstice_sun_pillbox pillbox;
  } radang_distrib;
};

static INLINE void
solstice_sun_init(struct mem_allocator* allocator, struct solstice_sun* sun)
{
  ASSERT(sun);
  sun->dni = 1.0;
  sun->radang_distrib_type = SOLSTICE_SUN_RADANG_DISTRIB_DIRECTIONAL;
  darray_spectrum_data_init(allocator, &sun->spectrum);
}

static INLINE void
solstice_sun_release(struct solstice_sun* sun)
{
  ASSERT(sun);
  darray_spectrum_data_release(&sun->spectrum);
}

static INLINE res_T
solstice_sun_copy(struct solstice_sun* dst, const struct solstice_sun* src)
{
  ASSERT(dst && src);
  return darray_spectrum_data_copy(&dst->spectrum, &src->spectrum);
}

static INLINE res_T
solstice_sun_copy_and_release
  (struct solstice_sun* dst, struct solstice_sun* src)
{
  ASSERT(dst && src);
  return darray_spectrum_data_copy_and_release(&dst->spectrum, &src->spectrum);
}

static INLINE void
solstice_sun_clear(struct solstice_sun* sun)
{
  ASSERT(sun);
  darray_spectrum_data_clear(&sun->spectrum);
}

#endif /* SOLSTICE_SUN_H */

