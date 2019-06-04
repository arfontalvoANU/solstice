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

#ifndef SOLPARSER_MTL_DATA_H
#define SOLPARSER_MTL_DATA_H

#include "solparser_spectrum.h"

enum solparser_mtl_data_type {
  SOLPARSER_MTL_DATA_REAL,
  SOLPARSER_MTL_DATA_SPECTRUM
};

struct solparser_mtl_data {
  enum solparser_mtl_data_type type;
  union {
    double real;
    struct solparser_spectrum_id spectrum;
  } value;
};

#endif /* SOLPARSER_MTL_DATA_H */

