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

#include "solstice_core.h"
#include "solstice_core_node.h"
#include "solstice_core_device.h"

#include <rsys/mem_allocator.h>
#include <rsys/algorithm.h>

#include <solstice/ssol.h>
#include <solstice/sanim.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/

static void
node_release(ref_T* ref)
{
  struct score_device* dev;
  struct score_node* node = CONTAINER_OF(ref, struct score_node, ref);
  ASSERT(ref);
  dev = node->device;
  ASSERT(dev && dev->allocator);
  switch (node->type) {
  case NODE_TRACKING_TARGET:
    break;
  case NODE_GEOMETRY:
    if (node->data.geometry_node.solver_instance)
      SSOL(instance_ref_put(node->data.geometry_node.solver_instance));
    break;
  case NODE_PIVOT:
    break;
  default: FATAL("Unreachable code.\n"); break;
  }
  node_ref_put_children(&node->anim);
  SANIM(node_release(&node->anim));
  MEM_RM(dev->allocator, node);
  score_device_ref_put(dev);
}

struct data {
  const struct ssol_object* searched;
  struct ssol_instance* result;
};


/*******************************************************************************
* Local functions
******************************************************************************/
res_T
node_create
  (struct score_device* dev,
   struct score_node** out_node,
   enum node_type type)
{
  struct score_node* node = NULL;
  res_T res = RES_OK;

  ASSERT(dev && out_node && type < NODE_TYPES_COUNT__);

  node = MEM_CALLOC(dev->allocator, 1, sizeof(struct score_node));
  if (!node) {
    res = RES_MEM_ERR;
    goto error;
  }

  score_device_ref_get(dev);
  node->device = dev;
  ref_init(&node->ref);
  node->type = type;

exit:
  if (out_node) *out_node = node;
  return res;
error:
  if (node) {
    score_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

void
node_ref_put_children(struct sanim_node* node)
{
  size_t count, i;
  int init;
  ASSERT(node);
  SANIM(node_is_initialized(node, &init));
  if (!init) return;
  SANIM(node_get_children_count(node, &count));
  for (i = 0; i < count; i++) {
    struct sanim_node* child_;
    struct score_node *child;
    SANIM(node_get_child(node, i, &child_));
    child = CONTAINER_OF(child_, struct score_node, anim);
    score_node_ref_put(child);
  }
}

/*******************************************************************************
 * Exported score_node functions
 ******************************************************************************/
res_T
score_node_geometry_create
  (struct score_device* dev,
   struct score_node** geom)
{
  struct sanim_node* anim;
  res_T res = RES_OK;
  res = node_create(dev, geom, NODE_GEOMETRY);
  if (res != RES_OK) goto error;
  anim = &(*geom)->anim;
  res = sanim_node_initialize((*geom)->device->allocator, anim);
  if (res != RES_OK) goto error;
exit:
  return res;
error:
  if (geom && *geom) {
    score_node_ref_put(*geom);
    *geom = NULL;
  }
  goto exit;
}

res_T
score_node_pivot_create
  (struct score_device* dev,
   struct score_node** node)
{
  res_T res = RES_OK;
  res = node_create(dev, node, NODE_PIVOT);
  if (res != RES_OK) goto error;
exit:
  return res;
error:
  if (node && *node) {
    score_node_ref_put(*node);
    *node = NULL;
  }
  goto exit;
}

res_T
score_node_empty_create
  (struct score_device* dev,
   struct score_node** node)
{
  res_T res = RES_OK;
  res = node_create(dev, node, NODE_EMPTY);
  if (res != RES_OK) goto error;
exit:
  return res;
error:
  if (node && *node) {
    score_node_ref_put(*node);
    *node = NULL;
  }
  goto exit;
}

res_T
score_node_tracking_target_create
  (struct score_device* dev,
   struct score_node** node)
{
  res_T res = RES_OK;
  res = node_create(dev, node, NODE_TRACKING_TARGET);
  if (res != RES_OK) goto error;
  res = sanim_node_initialize((*node)->device->allocator, &(*node)->anim);
  if (res != RES_OK) goto error;
exit:
  return res;
error:
  if (node && *node) {
    score_node_ref_put(*node);
    *node = NULL;
  }
  goto exit;
}

res_T
score_node_geometry_setup
  (struct score_node* node,
   struct ssol_instance* instance)
{
  res_T res = RES_OK;
  ASSERT(node && instance && node->type == NODE_GEOMETRY);
  /* TODO: deal with multiple setups */
  res = sanim_node_initialize(node->device->allocator, &node->anim);
  if (res != RES_OK) goto error;
  node->data.geometry_node.solver_instance = instance;
  SSOL(instance_ref_put(instance));
exit:
  return res;
error:
  if (node->data.geometry_node.solver_instance) {
    SSOL(instance_ref_put(node->data.geometry_node.solver_instance));
  }
  if (node->anim.data) {
    SANIM(node_release(&node->anim));
  }
  goto exit;
}

res_T
score_node_pivot_setup
  (struct score_node* node,
   const struct sanim_pivot* pivot,
   const struct sanim_tracking* tracking)
{
  res_T res = RES_OK;
  ASSERT(node && pivot && tracking && node->type == NODE_PIVOT);
  /* TODO: deal with multiple setups */
  res = sanim_node_initialize_pivot(
    node->device->allocator, pivot, tracking, &node->anim);
  if (res != RES_OK) goto error;
exit:
  return res;
error:
  if (node->anim.data) {
    SANIM(node_release(&node->anim));
  }
  goto exit;
}

void
score_node_track_me
  (const struct score_node* node,
   struct sanim_tracking* tracking)
{
  ASSERT(node && tracking && node->type == NODE_TRACKING_TARGET);
  SANIM(node_track_me(&node->anim, tracking));
}

res_T
score_node_add_child
  (struct score_node* father,
   struct score_node* child)
{
  res_T res = RES_OK;
  ASSERT(father && child
    && father->type != NODE_TRACKING_TARGET);
  res = sanim_node_add_child(&father->anim, &child->anim);
  if (res != RES_OK) return res;
  score_node_ref_get(child);
  return RES_OK;
}

void
score_node_ref_get(struct score_node* node)
{
  ASSERT(node) ;
  ref_get(&node->ref);
}

void
score_node_ref_put(struct score_node* node)
{
  ASSERT(node);
  ref_put(&node->ref, node_release);
}

void
score_node_set_translation
  (struct score_node* node,
   const double translation[3])
{
  ASSERT(node && translation);
  SANIM(node_set_translation(&node->anim, translation));
}

void
score_node_get_translation
  (const struct score_node* node,
   double translation[3])
{
  ASSERT(node && translation);
  SANIM(node_get_translation(&node->anim, translation));
}

void
score_node_set_rotations
  (struct score_node* node,
   const double rotations[3])
{
  ASSERT(node && rotations
    && node->type != NODE_TRACKING_TARGET);
  SANIM(node_set_rotations(&node->anim, rotations));
}

void
score_node_get_rotations
  (const struct score_node* node,
   double rotations[3])
{
  ASSERT(node && rotations
    && node->type != NODE_TRACKING_TARGET);
  SANIM(node_get_rotations(&node->anim, rotations));
}

void
score_node_set_receiver
  (struct score_node* node,
   const int mask)
{
  ASSERT(node && node->type == NODE_GEOMETRY);
  node->data.geometry_node.receiver_mask = mask;
}

void
score_node_sample
  (struct score_node* node,
   const int sample)
{
  ASSERT(node && node->type == NODE_GEOMETRY);
  node->data.geometry_node.sample = sample;
}

