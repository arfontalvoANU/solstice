/* Copyright (C) 2016-2018 CNRS, 2018-2019 |Meso|Star>
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

#ifndef SOLPARSER_SPECTRUM_H
#define SOLPARSER_SPECTRUM_H

#include <rsys/dynamic_array.h>

struct solparser_spectrum_data {
  double wavelength;
  double data;
};

#define DARRAY_NAME spectrum_data
#define DARRAY_DATA struct solparser_spectrum_data
#include <rsys/dynamic_array.h>

struct solparser_spectrum {
  struct darray_spectrum_data data;
};

struct solparser_spectrum_id { size_t i; };

static INLINE void
solparser_spectrum_init
  (struct mem_allocator* allocator, struct solparser_spectrum* spectrum)
{
  ASSERT(spectrum);
  darray_spectrum_data_init(allocator, &spectrum->data);
}

static INLINE void
solparser_spectrum_release(struct solparser_spectrum* spectrum)
{
  ASSERT(spectrum);
  darray_spectrum_data_release(&spectrum->data);
}

static INLINE res_T
solparser_spectrum_copy
  (struct solparser_spectrum* dst,
   const struct solparser_spectrum* src)
{
  ASSERT(dst && src);
  return darray_spectrum_data_copy(&dst->data, &src->data);
}

static INLINE res_T
solparser_spectrum_copy_and_release
  (struct solparser_spectrum* dst,
   struct solparser_spectrum* src)
{
  ASSERT(dst && src);
  return darray_spectrum_data_copy_and_release(&dst->data, &src->data);
}

#endif /* SOLPARSER_SPECTRUM_H */

