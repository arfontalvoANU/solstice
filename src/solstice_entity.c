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

#include "solstice.h"
#include "solstice_c.h"

#include <solstice/ssol.h>
#include <solstice/sanim.h>

/*******************************************************************************
 * Helper function
 ******************************************************************************/
static res_T
update_instance_transform
  (const struct sanim_node* n, const double transform[12], void* data)
{
  struct solstice_node* node;
  ASSERT(n && transform);
  (void)data;

  node = CONTAINER_OF(n, struct solstice_node, anim);
  if(node->type != SOLSTICE_NODE_GEOMETRY) return RES_OK;
  return ssol_instance_set_transform(node->instance, transform);
}

static struct solstice_node*
create_empty_node
  (struct solstice* solstice, const struct solparser_entity* entity)
{
  struct solstice_node* node = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity);
  (void)entity;

  res = solstice_node_empty_create(solstice->allocator, &node);
  if(res != RES_OK) goto error;

exit:
  return node;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

static struct solstice_node*
create_geometry_node
  (struct solstice* solstice, const struct solparser_entity* entity)
{
  struct solstice_node* node = NULL;
  struct ssol_instance* instance = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity);

  res = solstice_instantiate_geometry
    (solstice, entity->data.geometry, &instance);
  if(res != RES_OK) goto error;

  res = solstice_node_geometry_create(solstice->allocator, instance, &node);
  if(res != RES_OK) goto error;

exit:
  if(instance) SSOL(instance_ref_put(instance));
  return node;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

static struct solstice_node*
create_pivot_node
  (struct solstice* solstice,
   const struct solparser_entity* entity)
{
  struct solstice_node* node = NULL;
  struct solstice_node* target = NULL;
  const struct solparser_pivot* parser_pivot = NULL;
  struct sanim_pivot anim_pivot = SANIM_PIVOT_NULL;
  struct sanim_tracking anim_tracking = SANIM_TRACKING_NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity);

  parser_pivot = solparser_get_pivot(solstice->parser, entity->data.pivot);

  /* Setup the pivot descriptor TODO: 2-axis pivots */
  anim_pivot.type = PIVOT_SINGLE_AXIS;
  d3_set(anim_pivot.data.pivot1.ref_normal, parser_pivot->normal);
  d3_set(anim_pivot.data.pivot1.ref_point, parser_pivot->point);

  /* Setup the tracking descriptor */
  switch (parser_pivot->target_type) {
    case SOLPARSER_TARGET_ANCHOR:
      anim_tracking.policy = TRACKING_NODE_TARGET;
      target = *htable_anchor_find
        (&solstice->anchors, &parser_pivot->target.anchor.i);
      solstice_node_target_get_tracking(target, &anim_tracking);
      break;
    case SOLPARSER_TARGET_DIRECTION:
      anim_tracking.policy = TRACKING_OUT_DIR;
      d3_set(anim_tracking.data.out_dir.u, parser_pivot->target.direction);
      break;
    case SOLPARSER_TARGET_POSITION:
      anim_tracking.policy = TRACKING_POINT;
      d3_set(anim_tracking.data.point.target, parser_pivot->target.position);
      anim_tracking.data.point.target_is_local = 0; /* TODO */
      break;
    case SOLPARSER_TARGET_SUN:
      anim_tracking.policy = TRACKING_SUN;
      break;
    default: FATAL("Unreachable code.\n"); break;
  }

  res = solstice_node_pivot_create
    (solstice->allocator, &anim_pivot, &anim_tracking, &node);
  if(res != RES_OK) goto error;

  res = darray_nodes_push_back(&solstice->pivots, &node);
  if(res != RES_OK) goto error;

exit:
  return node;
error:
  if(node) {
    solstice_node_ref_put(node);
    node = NULL;
  }
  goto exit;
}

static struct solstice_node*
create_node(struct solstice* solstice, const struct solparser_entity* entity)
{
  struct solstice_node* node = NULL;
  struct solstice_node* tgt = NULL;
  struct solstice_node* child = NULL;
  size_t i;
  res_T res = RES_OK;
  ASSERT(solstice && entity);

  /* Create the entity node */
  switch(entity->type) {
    case SOLPARSER_ENTITY_EMPTY:
      node = create_empty_node(solstice, entity);
      break;
    case SOLPARSER_ENTITY_GEOMETRY:
      node = create_geometry_node(solstice, entity);
      break;
    case SOLPARSER_ENTITY_PIVOT:
      node = create_pivot_node(solstice, entity);
      break;
    default: FATAL("Unreachable code.\n"); break;
  }
  if(!node) {
    fprintf(stderr, "Could not setup the entity node.\n");
    goto error;
  }

  /* Setup the entity transform */
  solstice_node_set_translation(node, entity->translation);
  solstice_node_set_rotations(node, entity->rotation);

  /* Register entity anchors */
  FOR_EACH(i, 0, solparser_entity_get_anchors_count(entity)) {
    struct solparser_anchor_id id;
    const struct solparser_anchor* anchor = NULL;

    res = solstice_node_target_create(solstice->allocator, &tgt);
    if(res != RES_OK) goto error;

    id = solparser_entity_get_anchor(entity, i);
    anchor = solparser_get_anchor(solstice->parser, id);

    res = htable_anchor_set(&solstice->anchors, &id.i, &tgt);
    if(res != RES_OK) goto error;

    solstice_node_set_translation(tgt, anchor->position);

    res = solstice_node_add_child(node, tgt);
    if(res != RES_OK) goto error;

    tgt = NULL;
  }

  /* Setup children */
  FOR_EACH(i, 0, solparser_entity_get_children_count(entity)) {
    struct solparser_entity_id id;
    const struct solparser_entity* child_entity;

    id = solparser_entity_get_child(entity, i);
    child_entity = solparser_get_entity(solstice->parser, id);

    child = create_node(solstice, child_entity);
    if(!child) goto error;

    res = solstice_node_add_child(node, child);
    if(res != RES_OK) goto error;

    solstice_node_ref_put(child);
    child = NULL;
  }

exit:
  return node;
error:
  if(tgt) solstice_node_ref_put(tgt);
  if(child) solstice_node_ref_put(child);
  if(node) solstice_node_ref_put(node);
  node = NULL;
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_setup_entities(struct solstice* solstice)
{
  struct solparser_entity_iterator it, it_end;
  struct solstice_node* root = NULL;
  const double dummy_sun_dir[3] = {0, 0, -1};
  res_T res = RES_OK;
  ASSERT(solstice);

  /* Release possible previous roots (incomplete, TODO) */
  /*score_scene_clear(solstice->score);*/

  /* (re) create the list of roots from entities */
  solparser_entity_iterator_begin(solstice->parser, &it);
  solparser_entity_iterator_end(solstice->parser, &it_end);
  while(!solparser_entity_iterator_eq(&it, &it_end)) {
    struct solparser_entity_id entity_id;
    const struct solparser_entity* entity;

    entity_id = solparser_entity_iterator_get(&it);
    entity = solparser_get_entity(solstice->parser, entity_id);

    root = create_node(solstice, entity);
    if(!root) goto error;

    /* Initialialised the world space position of the entity geometry */
    res = sanim_node_visit_tree
      (&root->anim, dummy_sun_dir, NULL, update_instance_transform);
    if(res != RES_OK) {
      fprintf(stderr,
        "Could not setup the transformation of the entity geometries.\n");
      goto error;
    }

    res = darray_nodes_push_back(&solstice->roots, &root);
    if(res != RES_OK) {
      fprintf(stderr, "Could not register a root entity.\n");
      goto error;
    }

    solparser_entity_iterator_next(&it);
    root = NULL;
  }

exit:
  return res;
error:
  if(root) solstice_node_ref_put(root);
  goto exit;
}

res_T
solstice_update_entities(struct solstice* solstice, const double sun_dir[3])
{
  size_t i, n;
  res_T res = RES_OK;
  ASSERT(solstice && sun_dir);

  n = darray_nodes_size_get(&solstice->roots);
  FOR_EACH(i, 0, n) {
    struct solstice_node* node = darray_nodes_data_get(&solstice->roots)[i];

    /* Initialialised the world space position of the entity geometry */
    res = sanim_node_visit_tree
      (&node->anim, sun_dir, NULL, update_instance_transform);
    if(res != RES_OK) {
      fprintf(stderr,
        "Could not update the transformation of the entity geometries.\n");
      goto error;
    }
  }

exit:
  return res;
error:
  goto exit;
}

