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

#ifndef SOLSTICE_MATERIAL_H
#define SOLSTICE_MATERIAL_H

enum solstice_material_type {
  SOLSTICE_MATERIAL_MATTE,
  SOLSTICE_MATERIAL_MIRROR
};

struct solstice_material_matte {
  double reflectivity; /* In [0, 1] */
};

struct solstice_material_mirror {
  double roughness; /* In [0, 1] */
  double reflectivity; /* In [0, 1] */
};

struct solstice_material {
  enum solstice_material_type type;
  union {
    struct solstice_material_matte matte;
    struct solstice_material_mirror mirror;
  } data;
};

struct solstice_material_double_sided {
  struct solstice_material* front;
  struct solstice_material* back;
};

#endif /* SOLSTICE_MATERIAL_H */ 
