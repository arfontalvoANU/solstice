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

#ifndef SOLPARSER_PIVOT_H
#define SOLPARSER_PIVOT_H

#include <rsys/double3.h>

enum solparser_target_type {
  SOLPARSER_TARGET_ANCHOR,
  SOLPARSER_TARGET_DIRECTION,
  SOLPARSER_TARGET_POSITION,
  SOLPARSER_TARGET_SUN,

  SOLPARSER_TARGET_TYPES_COUNT__
};

struct solparser_anchor_id { size_t i; };

struct solparser_anchor {
  struct str name;
  double position[3];
};

static INLINE void
solparser_anchor_init
  (struct mem_allocator* allocator, struct solparser_anchor* anchor)
{
  ASSERT(anchor);
  str_init(allocator, &anchor->name);
}

static INLINE void
solparser_anchor_release(struct solparser_anchor* anchor)
{
  ASSERT(anchor);
  str_release(&anchor->name);
}

static INLINE res_T
solparser_anchor_copy
  (struct solparser_anchor* dst, const struct solparser_anchor* src)
{
  ASSERT(dst && src);
  d3_set(dst->position, src->position);
  return str_copy(&dst->name, &src->name);
}

static INLINE res_T
solparser_anchor_copy_and_release
  (struct solparser_anchor* dst, struct solparser_anchor* src)
{
  ASSERT(dst && src);
  d3_set(dst->position, src->position);
  return str_copy_and_release(&dst->name, &src->name);
}

struct solparser_target {
  enum solparser_target_type type;
  union {
    double position[3]; /* World space position */
    double direction[3]; /* World space direction */
    struct solparser_anchor_id anchor;
  } data;
};
#define SOLPARSER_TARGET_NULL__ { SOLPARSER_TARGET_TYPES_COUNT__, {{0,0,0}} }
static const struct solparser_target SOLPARSER_TARGET_NULL =
  SOLPARSER_TARGET_NULL__;

struct solparser_pivot_id { size_t i; };

struct solparser_x_pivot {
  double ref_point[3];
  struct solparser_target target;
};

#define SOLPARSER_X_PIVOT_NULL__ { {0,0,0}, SOLPARSER_TARGET_NULL__ }
static const struct solparser_x_pivot SOLPARSER_X_PIVOT_NULL =
  SOLPARSER_X_PIVOT_NULL__;

static INLINE void
solparser_x_pivot_init
(struct mem_allocator* allocator, struct solparser_x_pivot* x_pivot)
{
  (void) allocator;
  ASSERT(x_pivot);
  *x_pivot = SOLPARSER_X_PIVOT_NULL;
}
struct solparser_xz_pivot {
  double spacing;
  double ref_point[3];
  struct solparser_target target;
};

#define SOLPARSER_XZ_PIVOT_NULL__ { 0, {0,0,0}, SOLPARSER_TARGET_NULL__ }
static const struct solparser_xz_pivot SOLPARSER_XZ_PIVOT_NULL =
  SOLPARSER_XZ_PIVOT_NULL__;

static INLINE void
solparser_xz_pivot_init
  (struct mem_allocator* allocator, struct solparser_xz_pivot* xz_pivot)
{
  (void) allocator;
  ASSERT(xz_pivot);
  *xz_pivot = SOLPARSER_XZ_PIVOT_NULL;
}

#endif /* SOLPARSER_PIVOT_H */

