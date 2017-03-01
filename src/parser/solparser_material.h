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

#include <stddef.h>

enum solparser_material_type {
  SOLPARSER_MATERIAL_MATTE,
  SOLPARSER_MATERIAL_MIRROR,
  SOLPARSER_MATERIAL_THIN_DIELECTRIC,
  SOLPARSER_MATERIAL_VIRTUAL
};

struct solparser_material_matte {
  double reflectivity; /* In [0, 1] */
};

struct solparser_material_matte_id { size_t i; };

struct solparser_material_mirror {
  double roughness; /* In [0, 1] */
  double reflectivity; /* In [0, 1] */
};

struct solparser_material_mirror_id { size_t i; };

struct solparser_material_thin_dielectric {
  double refractive_index;
  double thickness;
};

struct solparser_material_thin_dielectric_id { size_t i; };

struct solparser_material {
  enum solparser_material_type type;
  union {
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
