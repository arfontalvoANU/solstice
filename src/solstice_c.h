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

#ifndef SOLSTICE_C_H
#define SOLSTICE_C_H

#include "solstice.h"
#include "parser/solparser.h"

#include <rsys/ref_count.h>
#include <solstice/sanim.h>

enum solstice_node_type {
  SOLSTICE_NODE_GEOMETRY,
  SOLSTICE_NODE_TARGET,
  SOLSTICE_NODE_PIVOT,
  SOLSTICE_NODE_EMPTY,
  SOLSTICE_NODE_TYPES_COUNT__
};

struct solstice_node {
  struct sanim_node anim;
  enum solstice_node_type type;
  struct ssol_instance* instance; /* Available for geometry nodes */

  ref_T ref;
  struct mem_allocator* allocator;
};

struct ssol_instance;

extern LOCAL_SYM res_T
solstice_draw
  (struct solstice* solstice);

extern LOCAL_SYM res_T
solstice_setup_entities
  (struct solstice* solstice);

extern LOCAL_SYM res_T
solstice_update_entities
  (struct solstice* solstice,
   const double sun_dir[3]);

extern LOCAL_SYM res_T
solstice_create_ssol_material
  (struct solstice* solstice,
   const struct solparser_material_id mtl_id,
   struct ssol_material** mtl);

extern LOCAL_SYM res_T
solstice_instantiate_geometry
  (struct solstice* solstice,
   const struct solparser_geometry_id geom_id,
   struct ssol_instance** inst);

extern LOCAL_SYM res_T
solstice_node_geometry_create
  (struct mem_allocator* allocator,
   struct ssol_instance* instance,
   struct solstice_node** node);

extern LOCAL_SYM res_T
solstice_node_empty_create
  (struct mem_allocator* allocator,
   struct solstice_node** node);

extern LOCAL_SYM res_T
solstice_node_pivot_create
  (struct mem_allocator* allocator,
   const struct sanim_pivot* pivot,
   const struct sanim_tracking* tracking,
   struct solstice_node** node);

extern LOCAL_SYM res_T
solstice_node_target_create
  (struct mem_allocator* allocator,
   struct solstice_node** node);

extern LOCAL_SYM void
solstice_node_ref_get
  (struct solstice_node* node);

extern LOCAL_SYM void
solstice_node_ref_put
  (struct solstice_node* node);

extern LOCAL_SYM void
solstice_node_target_get_tracking
  (const struct solstice_node* node,
   struct sanim_tracking* tracking);

extern LOCAL_SYM res_T
solstice_node_add_child
  (struct solstice_node* node,
   struct solstice_node* child);

static INLINE void
solstice_node_set_translation
  (struct solstice_node* node,
   const double translation[3])
{
  ASSERT(node && translation);
  SANIM(node_set_translation(&node->anim, translation));
}

static INLINE void
solstice_node_set_rotations
  (struct solstice_node* node,
   const double rotations[3]) /* In radians */
{
  ASSERT(node && rotations && node->type != SOLSTICE_NODE_TARGET);
  SANIM(node_set_rotations(&node->anim, rotations));
}

#endif /* SOLSTICE_C_H */

