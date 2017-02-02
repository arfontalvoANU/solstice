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

static INLINE int
srcvl_side_to_ssol_mask(const enum srcvl_side side)
{
  int mask = 0;
  switch(side) {
    case SRCVL_BACK: mask = SSOL_BACK; break;
    case SRCVL_FRONT: mask = SSOL_FRONT; break;
    case SRCVL_FRONT_AND_BACK: mask = SSOL_BACK | SSOL_FRONT; break;
    default: FATAL("Unreachable code.\n"); break;
  }
  return mask;
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
create_x_pivot_node
  (struct solstice* solstice,
   const struct solparser_entity* entity)
{
  double n[3];
  struct solstice_node* node = NULL;
  struct solstice_node* target = NULL;
  const struct solparser_x_pivot* parser_x_pivot = NULL;
  struct sanim_pivot anim_pivot = SANIM_PIVOT_NULL;
  struct sanim_tracking anim_tracking = SANIM_TRACKING_NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity);

  parser_x_pivot = solparser_get_x_pivot(solstice->parser, entity->data.x_pivot);

  anim_pivot.type = PIVOT_SINGLE_AXIS;
  d3_set(anim_pivot.data.pivot1.ref_normal, d3(n, 0, 0, 1));
  d3_set(anim_pivot.data.pivot1.ref_point, parser_x_pivot->ref_point);

  /* Setup the tracking descriptor */
  switch (parser_x_pivot->target.type) {
    case SOLPARSER_TARGET_ANCHOR:
      anim_tracking.policy = TRACKING_NODE_TARGET;
      target = *htable_anchor_find
        (&solstice->anchors, &parser_x_pivot->target.data.anchor.i);
      solstice_node_target_get_tracking(target, &anim_tracking);
      break;
    case SOLPARSER_TARGET_DIRECTION:
      anim_tracking.policy = TRACKING_OUT_DIR;
      d3_set(anim_tracking.data.out_dir.u, parser_x_pivot->target.data.direction);
      break;
    case SOLPARSER_TARGET_POSITION:
      anim_tracking.policy = TRACKING_POINT;
      d3_set(anim_tracking.data.point.target, parser_x_pivot->target.data.position);
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
create_xz_pivot_node
  (struct solstice* solstice,
   const struct solparser_entity* entity)
{
  struct solstice_node* node = NULL;
  struct solstice_node* target = NULL;
  const struct solparser_xz_pivot* parser_xz_pivot = NULL;
  struct sanim_pivot anim_pivot = SANIM_PIVOT_NULL;
  struct sanim_tracking anim_tracking = SANIM_TRACKING_NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity);

  parser_xz_pivot = solparser_get_xz_pivot(solstice->parser, entity->data.xz_pivot);

  anim_pivot.type = PIVOT_TWO_AXIS;
  anim_pivot.data.pivot2.spacing = parser_xz_pivot->spacing;
  d3_set(anim_pivot.data.pivot2.ref_point, parser_xz_pivot->ref_point);

  /* Setup the tracking descriptor */
  switch (parser_xz_pivot->target.type) {
  case SOLPARSER_TARGET_ANCHOR:
    anim_tracking.policy = TRACKING_NODE_TARGET;
    target = *htable_anchor_find
    (&solstice->anchors, &parser_xz_pivot->target.data.anchor.i);
    solstice_node_target_get_tracking(target, &anim_tracking);
    break;
  case SOLPARSER_TARGET_DIRECTION:
    anim_tracking.policy = TRACKING_OUT_DIR;
    d3_set(anim_tracking.data.out_dir.u, parser_xz_pivot->target.data.direction);
    break;
  case SOLPARSER_TARGET_POSITION:
    anim_tracking.policy = TRACKING_POINT;
    d3_set(anim_tracking.data.point.target, parser_xz_pivot->target.data.position);
    anim_tracking.data.point.target_is_local = 0; /* TODO */
    break;
  case SOLPARSER_TARGET_SUN:
    anim_tracking.policy = TRACKING_SUN;
    break;
  default: FATAL("Unreachable code.\n"); break;
  }

  res = solstice_node_pivot_create
    (solstice->allocator, &anim_pivot, &anim_tracking, &node);
  if (res != RES_OK) goto error;

  res = darray_nodes_push_back(&solstice->pivots, &node);
  if (res != RES_OK) goto error;

exit:
  return node;
error:
  if (node) {
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
  struct solstice_receiver* rcv = NULL;
  double rotation[3];
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
    case SOLPARSER_ENTITY_X_PIVOT:
      node = create_x_pivot_node(solstice, entity);
      break;
    case SOLPARSER_ENTITY_XZ_PIVOT:
      node = create_xz_pivot_node(solstice, entity);
      break;
    default: FATAL("Unreachable code.\n"); break;
  }
  if(!node) {
    fprintf(stderr, "Could not setup the entity node.\n");
    goto error;
  }

  /* Setup the primary parameter for the geometry entity */
  if(entity->type == SOLPARSER_ENTITY_GEOMETRY) {
    res = solstice_node_geometry_set_primary(node, entity->primary);
    if(res != RES_OK) {
      fprintf(stderr,
        "Could not define the primary parameter of the entity `%s'.\n",
        str_cget(&entity->name));
      goto error;
    }
  }

  /* Setup the entity receiver flags */
  rcv = htable_receiver_find(&solstice->receivers, &entity);
  if(rcv) {
    const int mask = srcvl_side_to_ssol_mask(rcv->side);
    ASSERT(rcv->node == NULL); /* Receiver is not attached to a node */

    res = solstice_node_geometry_set_receiver(node, mask);
    if(res != RES_OK) {
      fprintf(stderr, "Could not define the entity `%s' as a receiver.\n",
        str_cget(&entity->name));
      goto error;
    }
    rcv->node = node;
  }

  /* Setup the entity transform */
  rotation[0] = MDEG2RAD(entity->rotation[0]);
  rotation[1] = MDEG2RAD(entity->rotation[1]);
  rotation[2] = MDEG2RAD(entity->rotation[2]);
  solstice_node_set_translation(node, entity->translation);
  solstice_node_set_rotations(node, rotation);

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

    solstice_node_ref_put(tgt);
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

  /* (re) create the list of roots from entities */
  solparser_entity_iterator_begin(solstice->parser, &it);
  solparser_entity_iterator_end(solstice->parser, &it_end);
  while(!solparser_entity_iterator_eq(&it, &it_end)) {
    struct solparser_entity_id entity_id;
    const struct solparser_entity* entity;

    entity_id = solparser_entity_iterator_get(&it);
    entity = solparser_get_entity(solstice->parser, entity_id);

    root = create_node(solstice, entity);
    if(!root) {
      res = RES_BAD_ARG;
      goto error;
    }

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

