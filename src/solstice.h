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

#ifndef SOLSTICE_H
#define SOLSTICE_H

#include "parser/solparser_material.h"

#include <rsys/hash_table.h>
#include <rsys/mem_allocator.h>

struct solparser;
struct ssol_device;
struct ssol_material;
struct ssol_object;

#define HTABLE_NAME material
#define HTABLE_KEY size_t
#define HTABLE_DATA struct ssol_material*
#include <rsys/hash_table.h>

#define HTABLE_NAME object
#define HTABLE_KEY size_t
#define HTABLE_DATA struct ssol_object*
#include <rsys/hash_table.h>

struct solstice {
  struct ssol_device* ssol;
  struct solparser* parser;

  struct htable_material materials;
  struct htable_object objects;
};

extern LOCAL_SYM res_T
solstice_init
  (struct mem_allocator* allocator, /* May be NULL <=> use default allocator */
   struct solstice* solstice);

extern LOCAL_SYM void
solstice_release
  (struct solstice* solstice);

#endif /* SOLSTICE_H */

