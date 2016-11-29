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
  case NODE_TEMPLATE_ROOT:
    break;
  case NODE_INSTANCE_ROOT:
    darray_nodes_release(&node->data.instance_root.pivots);
    if (node->data.instance_root.template)
      score_node_ref_put(node->data.instance_root.template);
    break;
  case NODE_TRACKING_TARGET:
    break;
  case NODE_TEMPLATE:
    if (node->data.template_node.solver_object)
      SSOL(object_ref_put(node->data.template_node.solver_object));
    break;
  case NODE_INSTANCE:
    if (node->data.instance_node.solver_instance)
      SSOL(instance_ref_put(node->data.instance_node.solver_instance));
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

static res_T
search(const struct sanim_node* n, void* data_, int* found)
{
  const struct score_node* node;
  struct data* data;
  ASSERT(n && data_ && found);

  data = data_;
  node = CONTAINER_OF(n, struct score_node, anim);
  if (node->type == NODE_INSTANCE) {
    const struct ssol_object* solver_model;
    ASSERT(node->data.instance_node.model->type == NODE_TEMPLATE);
    solver_model = node->data.instance_node.model->data.template_node.solver_object;
    if (solver_model == data->searched) {
      data->result = node->data.instance_node.solver_instance;
      *found = 1;
      return RES_OK;
    }
  }
  return RES_OK;
}

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

static res_T
instance_internal_node_create
  (struct score_device* dev, struct score_node** node)
{
  struct sanim_node* anim;
  res_T res = RES_OK;
  res = node_create(dev, node, NODE_INSTANCE);
  if (res != RES_OK) goto error;
  anim = &(*node)->anim;
  res = sanim_node_initialize((*node)->device->allocator, anim);
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
node_instanciate_any
  (struct score_node* tp_node,
   struct score_node** out_node)
{
  struct score_node* node = NULL;
  struct score_device* dev;
  double v[3];
  res_T res = RES_OK;

  ASSERT(tp_node && out_node);

  dev = tp_node->device;
  ASSERT(dev && dev->allocator);

  switch (tp_node->type) {
  case NODE_TEMPLATE_ROOT:
    res = score_node_instantiate(tp_node, &node);
    if (res != RES_OK) goto error;
    break;
  case NODE_INSTANCE_ROOT:
    ASSERT(0);
    goto error;
  case NODE_TRACKING_TARGET:
    res = score_node_tracking_target_create(dev, &node);
    if (res != RES_OK) goto error;
    SANIM(node_get_translation(&tp_node->anim, v));
    SANIM(node_set_translation(&node->anim, v));
    /* rotations have no meaning for a target */
    break;
  case NODE_TEMPLATE:
    res = instance_internal_node_create(dev, &node);
    if (res != RES_OK) goto error;
    res = ssol_object_instantiate(
      tp_node->data.template_node.solver_object,
      &node->data.instance_node.solver_instance);
    if (res != RES_OK) goto error;
    node->data.instance_node.model = tp_node;
    node->data.instance_node.receiver_mask
      = tp_node->data.template_node.receiver_mask;
    node->data.instance_node.sample
      = tp_node->data.template_node.sample;
    SANIM(node_get_translation(&tp_node->anim, v));
    SANIM(node_set_translation(&node->anim, v));
    SANIM(node_get_rotations(&tp_node->anim, v));
    SANIM(node_set_rotations(&node->anim, v));
    break;
  case NODE_INSTANCE:
    ASSERT(0);
    goto error;
  case NODE_PIVOT:
#ifndef NDEBUG
  {
    int p;
    ASSERT(sanim_node_is_pivot(&tp_node->anim, &p) == RES_OK && p);
  }
#endif
    res = score_node_pivot_create(dev, &node);
    if (res != RES_OK) goto error;
    res = sanim_node_copy_initialize(
      dev->allocator, &tp_node->anim, &node->anim);
    /* copy includes translation and rotations */
    if (res != RES_OK) goto error;
    break;
  default: FATAL("Unreachable code.\n"); break;
  }

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

/*******************************************************************************
 * Exported score_node functions
 ******************************************************************************/
res_T
score_node_template_create
  (struct score_device* dev,
   struct score_node** node)
{
  struct sanim_node* anim;
  res_T res = RES_OK;
  res = node_create(dev, node, NODE_TEMPLATE_ROOT);
  if (res != RES_OK) goto error;
  anim = &(*node)->anim;
  res = sanim_node_initialize((*node)->device->allocator, anim);
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
score_node_create_object
  (struct score_device* dev,
   struct score_node** node)
{
  res_T res = RES_OK;
  res = node_create(dev, node, NODE_TEMPLATE);
  if (res != RES_OK) goto error;
  (*node)->data.template_node.sample = 1;
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
score_node_instantiate
  (struct score_node* template,
   struct score_node** out_instance)
{
  res_T res = RES_OK;
  struct score_device* dev;
  struct score_node* instance;
  ASSERT(template && out_instance && template->type == NODE_TEMPLATE_ROOT);
  dev = template->device;
  ASSERT(dev);
  res = node_create(dev, &instance, NODE_INSTANCE_ROOT);
  if (res != RES_OK) goto error;
  res = sanim_node_initialize(instance->device->allocator, &instance->anim);
  if (res != RES_OK) goto error;
  /* actual instantiation is deferred */
  instance->data.instance_root.template = template;
  darray_nodes_init(dev->allocator, &instance->data.instance_root.pivots);
  score_node_ref_get(template);
exit:
  *out_instance = instance;
  return res;
error:
  if (instance) {
    score_node_ref_put(instance);
    instance = NULL;
  }
  goto exit;
}

res_T
score_node_object_setup
  (struct score_node* node,
   struct ssol_object* object)
{
  res_T res = RES_OK;
  ASSERT(node && object && node->type == NODE_TEMPLATE);
  /* TODO: deal with multiple setups */
  res = sanim_node_initialize(node->device->allocator, &node->anim);
  if (res != RES_OK) goto error;
  node->data.template_node.solver_object = object;
  SSOL(object_ref_get(object));
exit:
  return res;
error:
  if (node->data.template_node.solver_object) {
    SSOL(object_ref_put(node->data.template_node.solver_object));
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
    && father->type != NODE_INSTANCE_ROOT
    && father->type != NODE_TRACKING_TARGET
    && child->type != NODE_TEMPLATE_ROOT
    && child->type != NODE_INSTANCE_ROOT
    );
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
  ASSERT(node && translation && node->type != NODE_TEMPLATE_ROOT);
  SANIM(node_set_translation(&node->anim, translation));
}

void
score_node_get_translation
  (const struct score_node* node,
   double translation[3])
{
  ASSERT(node && translation && node->type != NODE_TEMPLATE_ROOT);
  SANIM(node_get_translation(&node->anim, translation));
}

void
score_node_set_rotations
  (struct score_node* node,
   const double rotations[3])
{
  ASSERT(node && rotations
    && node->type != NODE_TEMPLATE_ROOT
    && node->type != NODE_TRACKING_TARGET);
  SANIM(node_set_rotations(&node->anim, rotations));
}

void
score_node_get_rotations
  (const struct score_node* node,
   double rotations[3])
{
  ASSERT(node && rotations
    && node->type != NODE_TEMPLATE_ROOT
    && node->type != NODE_TRACKING_TARGET);
  SANIM(node_get_rotations(&node->anim, rotations));
}

void
score_node_set_receiver
  (struct score_node* node,
   const int mask)
{
  ASSERT(node && node->type == NODE_TEMPLATE);
  node->data.template_node.receiver_mask = mask;
}

void
score_node_sample
  (struct score_node* node,
   const int sample)
{
  ASSERT(node && node->type == NODE_TEMPLATE);
  node->data.template_node.sample = sample;
}

void
score_node_get_instance_of
  (const struct score_node* instance,
   const struct score_node* node,
   struct ssol_instance** solver)
{
  struct data data;
  int found = 0;
  ASSERT(instance && node && solver
    && instance->type == NODE_INSTANCE_ROOT
    && node->type == NODE_TEMPLATE);
  
  data.searched = node->data.template_node.solver_object;
  data.result = NULL;
  SANIM(node_search_tree(&instance->anim, &data, search, &found));
  ASSERT(data.result);
  *solver = data.result;
}
