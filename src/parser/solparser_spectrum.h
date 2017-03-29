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

#endif /* SOLPARSER_SPECTRUM_H */

