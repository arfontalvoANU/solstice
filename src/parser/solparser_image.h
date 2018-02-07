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

#ifndef SOLPARSER_IMAGE_H
#define SOLPARSER_IMAGE_H

#include <rsys/str.h>

struct solparser_image {
  struct str filename;
};

struct solparser_image_id { size_t i; };

static INLINE void
solparser_image_init
  (struct mem_allocator* allocator,
   struct solparser_image* img)
{
  ASSERT(img);
  str_init(allocator, &img->filename);
}

static INLINE void
solparser_image_release(struct solparser_image* img)
{
  ASSERT(img);
  str_release(&img->filename);
}

static INLINE res_T
solparser_image_copy
  (struct solparser_image* dst,
   const struct solparser_image* src)
{
  ASSERT(dst && src);
  return str_copy(&dst->filename, &src->filename);
}

static INLINE res_T
solparser_image_copy_and_release
  (struct solparser_image* dst,
   struct solparser_image* src)
{
  ASSERT(dst && src);
  return str_copy_and_release(&dst->filename, &src->filename);
}

#endif /* SOLPARSER_IMAGE_H */

