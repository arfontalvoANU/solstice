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
#include "solstice_core_scene.h"
#include "solstice_core_node.h"
#include "solstice_core_device.h"

#include <rsys/double3.h>

#include <solstice/ssol.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/

FINLINE struct sanim_node**
contains(struct darray_nodes* array, struct sanim_node* elt)
{
  size_t count, i;
  struct sanim_node** data;
  ASSERT(array && elt);
  count = darray_nodes_size_get(array);
  data = darray_nodes_data_get(array);
  for (i = 0; i < count; i++) {
    if (elt == data[i]) return &data[i];
  }
  return NULL;
}

static int
remove_if(struct darray_nodes* array, struct sanim_node* elt)
{
  size_t count;
  struct sanim_node** data;
  struct sanim_node** ptr;
  ASSERT(array && elt);
  ptr = contains(array, elt);
  if (ptr == NULL) return 0;
  count = darray_nodes_size_get(array);
  data = darray_nodes_data_get(array);
  *ptr = data[count - 1];
  darray_nodes_pop_back(array);
  return 1;
}

static res_T
create_instance_tree_
  (struct darray_nodes* pivots,
   const struct score_node* t_node,
   struct score_node* i_node)
{
  res_T res = RES_OK;
  size_t count, i;
  struct score_node* node = NULL;
  int p;
  ASSERT(pivots && t_node && i_node);

  SANIM(node_get_children_count(&t_node->anim, &count));
  for (i = 0; i < count; i++) {
    struct sanim_node* t_child_;
    struct score_node *t_child, *i_child;
    SANIM(node_get_child(&t_node->anim, i, &t_child_));
    t_child = CONTAINER_OF(t_child_, struct score_node, anim);
    res = node_instanciate_any(t_child, &i_child);
    if (res != RES_OK) goto error;
    res = sanim_node_add_child(&i_node->anim, &i_child->anim);
    if (res != RES_OK) goto error;
    create_instance_tree_(pivots, t_child, i_child);
  }

  SANIM(node_is_pivot(&i_node->anim, &p));
  if (p) {
    struct sanim_node* n = &i_node->anim;
    res = darray_nodes_push_back(pivots, &n);
    if (res != RES_OK) goto error;
  }

exit:
  return res;
error:
  if (node) {
    node = NULL;
  }
  goto exit;
}

static res_T
create_instance_tree(struct score_node* instance)
{
  res_T res = RES_OK;
  struct score_node* temp;
  ASSERT(instance && instance->type == NODE_INSTANCE_ROOT);
  node_ref_put_children(&instance->anim);
  darray_nodes_clear(&instance->data.instance_root.pivots);
  temp = instance->data.instance_root.template;
  ASSERT(temp->type == NODE_TEMPLATE_ROOT);
  res = create_instance_tree_(&instance->data.instance_root.pivots, temp, instance);
  if (res != RES_OK) goto error;
exit:
  return res;
error:
  node_ref_put_children(&instance->anim);
  darray_nodes_clear(&instance->data.instance_root.pivots);
  goto exit;
}

static res_T
node_to_solver(const struct sanim_node* node_, const double transform[12], void* data)
{
  struct score_node* node;
  struct ssol_scene* scene = data;
  res_T res = RES_OK;
  ASSERT(node_ && transform && data);
  node = CONTAINER_OF(node_, struct score_node, anim);
  switch (node->type) {
  case NODE_TEMPLATE_ROOT:
    ASSERT(0);
    break;
  case NODE_INSTANCE_ROOT:
    /* the root doesn't include any solver-related item */
    break;
  case NODE_TRACKING_TARGET:
    /* not a solver-related item */
    break;
  case NODE_TEMPLATE:
    ASSERT(0);
    break;
  case NODE_INSTANCE:
    SSOL(instance_set_transform(
      node->data.instance_node.solver_instance, transform));
    SSOL(instance_set_receiver(
      node->data.instance_node.solver_instance,
      node->data.instance_node.receiver_mask));
    SSOL(instance_sample(
      node->data.instance_node.solver_instance,
      node->data.instance_node.sample));
    res = ssol_scene_attach_instance(
      scene, node->data.instance_node.solver_instance);
    break;
  case NODE_PIVOT:
    /* not a solver-related item */
    break;
  default: FATAL("Unreachable code.\n"); break;
  }
  return res;
}

static res_T
node_to_solver_update(const struct sanim_node* node_, const double transform[12], void* data)
{
  struct score_node* node;
  res_T res = RES_OK;
  (void) data;
  ASSERT(node_ && transform);
  node = CONTAINER_OF(node_, struct score_node, anim);
  switch (node->type) {
  case NODE_TEMPLATE_ROOT:
    ASSERT(0);
    break;
  case NODE_INSTANCE_ROOT:
    ASSERT(0); /* should only visit post-pivot nodes */
    break;
  case NODE_TRACKING_TARGET:
    /* not a solver-related item */
    break;
  case NODE_TEMPLATE:
    ASSERT(0);
    break;
  case NODE_INSTANCE:
    SSOL(instance_set_transform(
      node->data.instance_node.solver_instance, transform));
    break;
  case NODE_PIVOT:
    /* not a solver-related item */
    break;
  default: FATAL("Unreachable code.\n"); break;
  }
  return res;
}

static res_T
update_pivots(struct score_scene* scene, const double sun_dir[3])
{
  size_t i_count, i;
  struct sanim_node** instances;
  res_T res = RES_OK;
  ASSERT(scene && sun_dir);
  i_count = darray_nodes_size_get(&scene->instances);
  instances = darray_nodes_data_get(&scene->instances);
  for (i = 0; i < i_count; i++) {
    size_t p_count, p;
    struct sanim_node** pivots;
    struct score_node* instance = CONTAINER_OF(instances[i], struct score_node, anim);
    ASSERT(instance->type == NODE_INSTANCE_ROOT);
    p_count = darray_nodes_size_get(&instance->data.instance_root.pivots);
    pivots = darray_nodes_data_get(&instance->data.instance_root.pivots);
    for (p = 0; p < p_count; p++) {
      struct score_node* pivot = CONTAINER_OF(pivots[p], struct score_node, anim);
      res = sanim_node_solve_pivot(&pivot->anim, sun_dir);
      if (res != RES_OK) return res;
    }
  }
  return RES_OK;
}

static void
scene_release(ref_T* ref)
{
  struct score_device* dev;
  struct score_scene* scene = CONTAINER_OF(ref, struct score_scene, ref);
  ASSERT(ref);
  dev = scene->device;
  ASSERT(dev && dev->allocator);
  score_scene_clear(scene);
  darray_nodes_release(&scene->instances);
  if (scene->solver) SSOL(scene_ref_put(scene->solver));
  if (scene->sun) SSOL(sun_ref_put(scene->sun));
  if (scene->atmosphere) SSOL(atmosphere_ref_put(scene->atmosphere));
  MEM_RM(dev->allocator, scene);
  score_device_ref_put(dev);
}

/*******************************************************************************
 * Exported score_scene functions
 ******************************************************************************/
res_T
score_scene_create
  (struct score_device* dev,
   struct score_scene** out_scene)
{
  struct score_scene* scene;
  res_T res = RES_OK;
  ASSERT(dev && out_scene);

  scene = MEM_CALLOC(dev->allocator, 1, sizeof(struct score_scene));
  if (!scene) {
    res = RES_MEM_ERR;
    goto error;
  }

  score_device_ref_get(dev);
  scene->device = dev;
  ref_init(&scene->ref);
  darray_nodes_init(dev->allocator, &scene->instances);

  res = ssol_scene_create(dev->ssol, &scene->solver);
  if (res != RES_OK) goto error;

exit:
  if (out_scene) *out_scene = scene;
  return res;
error:
  if (scene) {
    score_scene_ref_put(scene);
    scene = NULL;
  }
  goto exit;
}

void
score_scene_ref_get(struct score_scene* scene)
{
  ASSERT(scene);
  ref_get(&scene->ref);
}

void
score_scene_ref_put(struct score_scene* scene)
{
  ASSERT(scene);
  ref_put(&scene->ref, scene_release);
}

res_T
score_scene_attach_instance
  (struct score_scene* scene,
   struct score_node* instance)
{
  struct sanim_node* anim;
  res_T res = RES_OK;
  int ok;
  ASSERT(scene && instance
    && instance->type == NODE_INSTANCE_ROOT
    && !instance->data.instance_root.scene_attachment);
  anim = &instance->anim;
  SANIM(node_is_initialized(anim, &ok));
  ASSERT(ok);
  res = darray_nodes_push_back(&scene->instances, &anim);
  if (res != RES_OK) goto error;
  instance->data.instance_root.scene_attachment = scene;
  score_node_ref_get(instance);
exit:
  return res;
error:
  remove_if(&scene->instances, anim);
  goto exit;
}

void
score_scene_detach_instance
  (struct score_scene* scene,
   struct score_node* instance)
{
  struct sanim_node* anim;
  int ok;
  ASSERT(scene && instance
    && instance->type == NODE_INSTANCE_ROOT
    && instance->data.instance_root.scene_attachment == scene);
  anim = &instance->anim;
  SANIM(node_is_initialized(anim, &ok));
  ASSERT(ok);
  ok = remove_if(&scene->instances, anim);
  ASSERT(ok);
  instance->data.instance_root.scene_attachment = NULL;
  score_node_ref_put(instance);
}

void
score_scene_attach_sun
  (struct score_scene* scene,
   struct ssol_sun* sun)
{
  ASSERT(scene && sun);
  SSOL(scene_attach_sun(scene->solver, sun));
  SSOL(sun_ref_get(sun));
  scene->sun = sun;
}

void
score_scene_detach_sun
  (struct score_scene* scene,
   struct ssol_sun* sun)
{
  ASSERT(scene && sun);
  SSOL(scene_detach_sun(scene->solver, sun));
  SSOL(sun_ref_put(sun));
  scene->sun = NULL;
}

void
score_scene_attach_atmosphere
  (struct score_scene* scene,
   struct ssol_atmosphere* atm)
{
  ASSERT(scene && atm);
  SSOL(scene_attach_atmosphere(scene->solver, atm));
  SSOL(atmosphere_ref_get(atm));
  scene->atmosphere = atm;
}

void
score_scene_detach_atmosphere
  (struct score_scene* scene,
   struct ssol_atmosphere* atm)
{
  ASSERT(scene && atm);
  SSOL(scene_detach_atmosphere(scene->solver, atm));
  SSOL(atmosphere_ref_put(atm));
  scene->atmosphere = NULL;
}

void
score_scene_clear
  (struct score_scene* scene)
{
  size_t count, i;
  struct sanim_node** data;
  ASSERT(scene);
  count = darray_nodes_size_get(&scene->instances);
  data = darray_nodes_data_get(&scene->instances);
  for (i = 0; i < count; i++) {
    struct score_node* node = CONTAINER_OF(data[i], struct score_node, anim);
    score_node_ref_put(node);
  }
  darray_nodes_clear(&scene->instances);
}

res_T
score_scene_reset_simulation(struct score_scene* scene)
{
  res_T res = RES_OK;
  size_t count = 0, i;
  struct sanim_node** instances;
  double sun_dir[3];
  ASSERT(scene && scene->sun);
  SSOL(sun_get_direction(scene->sun, sun_dir));
  count = darray_nodes_size_get(&scene->instances);
  instances = darray_nodes_data_get(&scene->instances);
  SSOL(scene_clear(scene->solver));
  SSOL(scene_attach_sun(scene->solver, scene->sun));
  if (scene->atmosphere)
    SSOL(scene_attach_atmosphere(scene->solver, scene->atmosphere));
  for (i = 0; i < count; i++) {
    struct score_node* inst = CONTAINER_OF(instances[i], struct score_node, anim);
    ASSERT(inst->type == NODE_INSTANCE_ROOT);
    res = create_instance_tree(inst);
    if (res != RES_OK) goto error;
    res = sanim_node_visit_tree(
      instances[i], sun_dir, scene->solver, &node_to_solver);
    if (res != RES_OK) goto error;
  }
exit:
  return res;
error:
  for (i = 0; i < count; i++) node_ref_put_children(instances[i]);
  goto exit;
}

res_T
score_scene_update_simulation(struct score_scene* scene)
{
  res_T res = RES_OK;
  size_t count = 0, i;
  size_t p_count = 0, p;
  struct sanim_node** instances;
  double sun_dir[3];
  ASSERT(scene && scene->sun);
  SSOL(sun_get_direction(scene->sun, sun_dir));
  count = darray_nodes_size_get(&scene->instances);
  instances = darray_nodes_data_get(&scene->instances);
  for (i = 0; i < count; i++) {
    struct sanim_node** pivots;
    struct score_node* inst = CONTAINER_OF(instances[i], struct score_node, anim);
    ASSERT(inst->type == NODE_INSTANCE_ROOT);
    p_count = darray_nodes_size_get(&inst->data.instance_root.pivots);
    pivots = darray_nodes_data_get(&inst->data.instance_root.pivots);
    for (p = 0; p < p_count; p++) {
      res = sanim_node_visit_tree(
        pivots[p], sun_dir, NULL, &node_to_solver_update);
      if (res != RES_OK) goto error;
    }
  }
exit:
  return res;
error:
  goto exit;
}
