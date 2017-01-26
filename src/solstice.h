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

#include <rsys/dynamic_array_double.h>
#include <rsys/hash_table.h>
#include <rsys/mem_allocator.h>
#include <rsys/str.h>

struct solparser;
struct solstice_args;
struct solstice_node;
struct ssol_device;
struct ssol_material;
struct ssol_object;
struct sanim_node;

struct solstice_receiver {
  struct solstice_node* node;
  struct str name; /* Absolute entity name */
  enum srcvl_side side;
};

static void
solstice_receiver_init
  (struct mem_allocator* allocator,
   struct solstice_receiver* receiver)
{
  ASSERT(allocator && receiver);
  receiver->node = NULL;
  receiver->side = SRCVL_FRONT_AND_BACK;
  str_init(allocator, &receiver->name);
}

static void
solstice_receiver_release(struct solstice_receiver* receiver)
{
  ASSERT(receiver);
  str_release(&receiver->name);
}

static res_T
solstice_receiver_copy
  (struct solstice_receiver* dst,
   const struct solstice_receiver* src)
{
  ASSERT(dst && src);
  dst->node = src->node;
  dst->side = src->side;
  return str_copy(&dst->name, &src->name);
}

static res_T
solstice_receiver_copy_and_release
  (struct solstice_receiver* dst, struct solstice_receiver* src)
{
  ASSERT(dst && src);
  dst->node = src->node;
  dst->side = src->side;
  return str_copy_and_release(&dst->name, &src->name);
}

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
#define HTABLE_KEY const struct solparser_entity*
#define HTABLE_DATA struct solstice_receiver
#define HTABLE_DATA_FUNCTOR_INIT solstice_receiver_init
#define HTABLE_DATA_FUNCTOR_RELEASE solstice_receiver_release
#define HTABLE_DATA_FUNCTOR_COPY solstice_receiver_copy
#define HTABLE_DATA_FUNCTOR_COPY_AND_RELEASE solstice_receiver_copy_and_release
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

  struct darray_double sun_dirs; /* List of double3 */

  size_t nrealisations; /* # realisations */
  FILE* output; /* Output stream */
  int output_hits; /* Output per receiver hits */

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

