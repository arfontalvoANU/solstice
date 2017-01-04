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

#ifndef SOLSTICE_H
#define SOLSTICE_H

#include "parser/solparser_material.h"

#include <rsys/hash_table.h>
#include <rsys/mem_allocator.h>

struct solparser;
struct solstice_args;
struct ssol_device;
struct ssol_material;
struct ssol_object;
struct sanim_node;
struct score_device;
struct score_node;

#define HTABLE_NAME material
#define HTABLE_KEY size_t
#define HTABLE_DATA struct ssol_material*
#include <rsys/hash_table.h>
#include <rsys/dynamic_array.h>

#define HTABLE_NAME object
#define HTABLE_KEY size_t
#define HTABLE_DATA struct ssol_object*
#include <rsys/hash_table.h>

#define HTABLE_NAME anchor
#define HTABLE_KEY size_t
#define HTABLE_DATA struct score_node*
#include <rsys/hash_table.h>

#include "core/solstice_core_node.h"

struct solstice {
  struct ssol_device* ssol;
  struct ssol_scene* scene;

  struct solparser* parser;
  struct score_device* score;

  struct htable_material materials;
  struct htable_object objects;
  struct htable_anchor anchors;
  struct darray_nodes roots;
  struct darray_nodes pivots;

  /* Rendering */
  struct ssol_camera* camera;
  struct ssol_image* framebuffer;

  struct mem_allocator* allocator;
};

extern LOCAL_SYM res_T
solstice_init
  (struct mem_allocator* allocator, /* May be NULL <=> use default allocator */
   const struct solstice_args* args,
   struct solstice* solstice);

extern LOCAL_SYM void
solstice_release
  (struct solstice* solstice);

extern LOCAL_SYM res_T
solstice_run
  (struct solstice* solstice);

#endif /* SOLSTICE_H */

