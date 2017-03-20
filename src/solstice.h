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
#include "receivers/srcvl.h"
#include "solstice_args.h"

#include <rsys/dynamic_array_double.h>
#include <rsys/hash_table.h>
#include <rsys/mem_allocator.h>
#include <rsys/str.h>

struct solparser;
struct solstice_node;
struct ssol_device;
struct ssol_material;
struct ssol_object;
struct sanim_node;

struct solstice_receiver {
  struct solstice_node* node;
  enum srcvl_side side;
  int per_primitive;
};

#define DARRAY_NAME nodes
#define DARRAY_DATA struct solstice_node*
#include <rsys/dynamic_array.h>

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
#define HTABLE_DATA struct solstice_node*
#include <rsys/hash_table.h>

#define HTABLE_NAME receiver
#define HTABLE_KEY struct str
#define HTABLE_KEY_FUNCTOR_INIT str_init
#define HTABLE_KEY_FUNCTOR_RELEASE str_release
#define HTABLE_KEY_FUNCTOR_COPY str_copy
#define HTABLE_KEY_FUNCTOR_COPY_AND_RELEASE str_copy_and_release
#define HTABLE_KEY_FUNCTOR_EQ str_eq
#define HTABLE_KEY_FUNCTOR_HASH str_hash
#define HTABLE_DATA struct solstice_receiver
#include <rsys/hash_table.h>

struct solstice {
  struct ssol_device* ssol;
  struct ssol_scene* scene;
  struct ssol_sun* sun;

  struct solparser* parser;

  struct htable_material materials;
  struct htable_object objects;
  struct htable_anchor anchors;
  struct htable_receiver receivers;
  struct darray_nodes roots;
  struct darray_nodes pivots;
  struct ssol_material* mtl_virtual; /* Shared virtual material */

  /* Rendering */
  struct ssol_camera* camera;
  struct ssol_image* framebuffer;
  enum solstice_args_render_mode render_mode;
  unsigned spp; /* #Samples per pixel */

  /* Dump geometry */
  enum solstice_args_dump_format dump_format;
  enum solstice_args_dump_split_mode dump_split_mode;

  struct darray_double sun_dirs; /* List of double3 */
  struct darray_double sun_angles;

  size_t nexperiments; /* # MC experiments */
  FILE* output; /* Output stream */
  int output_hits; /* Output per receiver hits */
  int dump_paths;

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

