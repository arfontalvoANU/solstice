/* Copyright (C) CNRS 2016-2018
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

#ifndef SOLPARSER_ATMOSPHERE_H
#define SOLPARSER_ATMOSPHERE_H

#include "solparser_mtl_data.h"

struct solparser_atmosphere {
  struct solparser_mtl_data extinction;
};

static INLINE void
solparser_atmosphere_init
  (struct mem_allocator* allocator, struct solparser_atmosphere* atmosphere)
{
  ASSERT(atmosphere);
  (void)allocator, (void)atmosphere;
  /* Do nothing */
}

static INLINE void
solparser_atmosphere_release(struct solparser_atmosphere* atmosphere)
{
  ASSERT(atmosphere);
  (void)atmosphere;
  /* Do nothing */
}

static INLINE void
solparser_atmosphere_clear(struct solparser_atmosphere* atmosphere)
{
  ASSERT(atmosphere);
  (void)atmosphere;
  /* Do nothing */
}

#endif /* SOLPARSER_ATMOSPHERE_H */

