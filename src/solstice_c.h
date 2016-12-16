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

#ifndef SOLSTICE_C_H
#define SOLSTICE_C_H

#include "solstice.h"
#include "parser/solparser.h"

struct ssol_instance;

extern LOCAL_SYM res_T
solstice_setup_entities
  (struct solstice* solstice);

extern LOCAL_SYM res_T
solstice_get_ssol_material
  (struct solstice* solstice,
   const struct solparser_material_id mtl_id,
   struct ssol_material** mtl);

extern LOCAL_SYM res_T
solstice_instantiate_geometry
  (struct solstice* solstice,
   const struct solparser_geometry_id geom_id,
   struct ssol_instance** inst);

#endif /* SOLSTICE_C_H */

