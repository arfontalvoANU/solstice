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

#include "solstice_c.h"
#include <solstice/ssol.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static res_T
node_create
  (struct mem_allocator* allocator,
   const enum solstice_node_type type,
   struct solstice_node** out_node)
{
  struct solstice_node* node = NULL;
  res_T res = RES_OK;
  ASSERT(allocator && out_node && type < SOLSTICE_NODE_TYPES_COUNT__);

  node = MEM_CALLOC(allocator, 1, sizeof(struct solstice_node));
  if(!node) {
    fprintf(stderr, "Could not allocate a Solstice node.\n");
    res = RES_MEM_ERR;
    goto error;
  }

  ref_init(&node->ref);
  str_init(allocator, &node->name);
  node->type = type;
  node->anim = SANIM_NODE_NULL;
  node->allocator = allocator;

exit:
  if(out_node) *out_node = node;
  return res;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

static void
node_release(ref_T* ref)
{
  struct solstice_node* node;
  int is_init;
  ASSERT(ref);

  node = CONTAINER_OF(ref, struct solstice_node, ref);
  if(node->instance) SSOL(instance_ref_put(node->instance));
  str_release(&node->name);

  SANIM(node_is_initialized(&node->anim, &is_init));
  if(is_init) {
    size_t i, n;
    SANIM(node_get_children_count(&node->anim, &n));
    FOR_EACH(i, 0, n) {
      struct sanim_node* child_anim;
      struct solstice_node* child;
      SANIM(node_get_child(&node->anim, i, &child_anim));
      child = CONTAINER_OF(child_anim, struct solstice_node, anim);
      solstice_node_ref_put(child);
    }
    SANIM(node_release(&node->anim));
  }
  MEM_RM(node->allocator, node);
}

/*******************************************************************************
 * Exported functions
 ******************************************************************************/
res_T
solstice_node_geometry_create
  (struct mem_allocator* allocator,
   struct ssol_instance* instance,
   struct solstice_node** out_node)
{
  struct solstice_node* node = NULL;
  res_T res = RES_OK;
  ASSERT(allocator && instance && out_node);

  res = node_create(allocator, SOLSTICE_NODE_GEOMETRY, &node);
  if(res != RES_OK) goto error;

  res = sanim_node_initialize(allocator, &node->anim);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not initialize the anim field of a Solstice geometry node.\n");
    goto error;
  }

  SSOL(instance_ref_get(instance));
  node->instance = instance;

exit:
  *out_node = node;
  return res;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

res_T
solstice_node_empty_create
  (struct mem_allocator* allocator,
   struct solstice_node** out_node)
{
  struct solstice_node* node = NULL;
  res_T res = RES_OK;
  ASSERT(allocator && out_node);

  res = node_create(allocator, SOLSTICE_NODE_EMPTY, &node);
  if(res != RES_OK) goto error;

  res = sanim_node_initialize(allocator, &node->anim);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not initialize the anim field of a Solstice empty node.\n");
    goto error;
  }

exit:
  *out_node = node;
  return res;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

res_T
solstice_node_pivot_create
  (struct mem_allocator* allocator,
   const struct sanim_pivot* pivot,
   const struct sanim_tracking* tracking,
   struct solstice_node** out_node)
{
  struct solstice_node* node = NULL;
  res_T res = RES_OK;
  ASSERT(allocator && pivot && tracking && out_node);

  res = node_create(allocator, SOLSTICE_NODE_PIVOT, &node);
  if(res != RES_OK) goto error;

  res = sanim_node_initialize_pivot(allocator, pivot, tracking, &node->anim);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not initialize the anim field of a Solstice pivot node.\n");
    goto error;
  }

exit:
  *out_node = node;
  return res;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

res_T
solstice_node_target_create
  (struct mem_allocator* allocator,
   struct solstice_node** out_node)
{
  struct solstice_node* node = NULL;
  res_T res = RES_OK;
  ASSERT(allocator && out_node);

  res = node_create(allocator, SOLSTICE_NODE_TARGET, &node);
  if(res != RES_OK) goto error;

  res = sanim_node_initialize(allocator, &node->anim);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not initialize the anim field of a Solstice target node.\n");
    goto error;
  }

exit:
  *out_node = node;
  return res;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

void
solstice_node_ref_get(struct solstice_node* node)
{
  ASSERT(node);
  ref_get(&node->ref);
}

void
solstice_node_ref_put(struct solstice_node* node)
{
  ASSERT(node);
  ref_put(&node->ref, node_release);
}

res_T
solstice_node_set_name(struct solstice_node* node, const char* name)
{
  ASSERT(node);
  return str_set(&node->name, name);
}

const char*
solstice_node_get_name(const struct solstice_node* node)
{
  ASSERT(node);
  return str_cget(&node->name);
}

res_T
solstice_node_geometry_set_primary
  (struct solstice_node* node, const int is_primary)
{
  ASSERT(node && (!is_primary || node->type == SOLSTICE_NODE_GEOMETRY));
  return ssol_instance_sample(node->instance, is_primary);
}

res_T
solstice_node_geometry_set_receiver
  (struct solstice_node* node, const int mask, const int per_primitive)
{
  ASSERT(node && node->type == SOLSTICE_NODE_GEOMETRY);
  return ssol_instance_set_receiver(node->instance, mask, per_primitive);
}

void
solstice_node_target_get_tracking
  (const struct solstice_node* node,
   struct sanim_tracking* tracking)
{
  ASSERT(node && tracking && node->type == SOLSTICE_NODE_TARGET);
  SANIM(node_track_me(&node->anim, tracking));
}

res_T
solstice_node_add_child(struct solstice_node* node, struct solstice_node* child)
{
  res_T res = RES_OK;
  ASSERT(node && child && node->type != SOLSTICE_NODE_TARGET);
  res = sanim_node_add_child(&node->anim, &child->anim);
  if(res != RES_OK) {
    fprintf(stderr, "Could not add a child node.\n");
    return res;
  }
  solstice_node_ref_get(child);
  return RES_OK;
}

