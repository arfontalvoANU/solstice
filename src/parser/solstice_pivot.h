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

#ifndef SOLSTICE_PIVOT_H
#define SOLSTICE_PIVOT_H

#include <rsys/double3.h>

enum solstice_target_type {
  SOLSTICE_TARGET_ANCHOR,
  SOLSTICE_TARGET_DIRECTION,
  SOLSTICE_TARGET_POSITION,
  SOLSTICE_TARGET_SUN
};

struct solstice_anchor_id { size_t i; };

struct solstice_anchor {
  struct str name;
  double position[3];
};

static INLINE void
solstice_anchor_init
  (struct mem_allocator* allocator, struct solstice_anchor* anchor)
{
  ASSERT(anchor);
  str_init(allocator, &anchor->name);
}

static INLINE void
solstice_anchor_release(struct solstice_anchor* anchor)
{
  ASSERT(anchor);
  str_release(&anchor->name);
}

static INLINE res_T
solstice_anchor_copy
  (struct solstice_anchor* dst, const struct solstice_anchor* src)
{
  ASSERT(dst && src);
  d3_set(dst->position, src->position);
  return str_copy(&dst->name, &src->name);
}

static INLINE res_T
solstice_anchor_copy_and_release
  (struct solstice_anchor* dst, struct solstice_anchor* src)
{
  ASSERT(dst && src);
  d3_set(dst->position, src->position);
  return str_copy_and_release(&dst->name, &src->name);
}

struct solstice_pivot {
  double point[3];
  double normal[3];
  double rotation[3];
  double translation[3];
  enum solstice_target_type target_type;
  union {
    double position[3]; /* World space position */
    double direction[3]; /* World space direction */
    struct solstice_anchor_id anchor;
  } target;
};

struct solstice_pivot_id { size_t i; };

#endif /* SOLSTICE_PIVOT_H */

