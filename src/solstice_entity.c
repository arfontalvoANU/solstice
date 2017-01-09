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
#include "core/solstice_core.h"

/*******************************************************************************
 * Helper function
 ******************************************************************************/
static res_T
update_instance_transform
  (const struct sanim_node* n, const double transform[12], void* data)
{
  struct score_node* node;
  ASSERT(n && transform);
  (void)data;

  node = CONTAINER_OF(n, struct score_node, anim);
  if(node->type != NODE_GEOMETRY) return RES_OK;
  return ssol_instance_set_transform
    (node->data.geometry_node.solver_instance, transform);
}

static struct score_node*
setup_entity_empty
  (struct solstice* solstice, const struct solparser_entity* entity)
{
  struct score_node* empty_node = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity);

  res = score_node_empty_create(solstice->score, &empty_node);
  if(res != RES_OK) goto error;

  score_node_set_translation(empty_node, entity->translation);
  score_node_set_rotations(empty_node, entity->rotation);

exit:
  return empty_node;
error:
  if(empty_node) {
    score_node_ref_put(empty_node);
    empty_node = NULL;
  }
  goto exit;
}

static struct score_node*
setup_entity_geometry
  (struct solstice* solstice, const struct solparser_entity* entity)
{
  struct score_node* geometry_node = NULL;
  struct ssol_instance* instance = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity);

  res = score_node_geometry_create(solstice->score, &geometry_node);
  if(res != RES_OK) goto error;

  res = solstice_instantiate_geometry
    (solstice, entity->data.geometry, &instance);
  if(res != RES_OK) goto error;

  res = score_node_geometry_setup(geometry_node, instance);
  if(res != RES_OK) goto error;

  score_node_set_translation(geometry_node, entity->translation);
  score_node_set_rotations(geometry_node, entity->rotation);

exit:
  if(instance) SSOL(instance_ref_put(instance));
  return geometry_node;
error:
  if(geometry_node) {
    score_node_ref_put(geometry_node);
    geometry_node = NULL;
  }
  goto exit;
}

static struct score_node*
setup_entity_pivot
  (struct solstice* solstice,
   const struct solparser_entity* entity,
   struct score_node** atchmnt_node)
{
  /* TODO: remove pivot positionning? */
  struct score_node* entity_node = NULL;
  struct score_node* pivot_node = NULL;
  const struct solparser_pivot* parser_pivot = NULL;
  struct sanim_pivot anim_pivot = SANIM_PIVOT_NULL;
  struct sanim_tracking anim_tracking = SANIM_TRACKING_NULL;
  res_T res = RES_OK;
  ASSERT(solstice && entity && atchmnt_node);

  /*
   * Each entity introduces a new coordinate system and each object or pivot
   * has some positionning in this system. This behaviour is implemented
   * through 2 levels of sanim_node.
   */

  res = score_node_empty_create(solstice->score, &entity_node);
  if(res != RES_OK) goto error;
  score_node_set_translation(entity_node, entity->translation);
  score_node_set_rotations(entity_node, entity->rotation);

  parser_pivot = solparser_get_pivot(solstice->parser, entity->data.pivot);
  res = score_node_pivot_create(solstice->score, &pivot_node);
  if(res != RES_OK) goto error;

  /* TODO: 2-axis pivots */
  anim_pivot.type = PIVOT_SINGLE_AXIS;
  d3_set(anim_pivot.data.pivot1.ref_normal, parser_pivot->normal);
  d3_set(anim_pivot.data.pivot1.ref_point, parser_pivot->point);
  switch (parser_pivot->target_type) {
    case SOLPARSER_TARGET_ANCHOR:
    {
      struct score_node** ptgt = NULL;
      const struct solparser_anchor_id id = parser_pivot->target.anchor;
      anim_tracking.policy = TRACKING_NODE_TARGET;
      ptgt = htable_anchor_find(&solstice->anchors, &id.i);
      score_node_track_me(*ptgt, &anim_tracking);
      break;
    }
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

  res = score_node_pivot_setup(pivot_node, &anim_pivot, &anim_tracking);
  if(res != RES_OK) goto error;
  score_node_set_translation(pivot_node, parser_pivot->translation);
  score_node_set_rotations(pivot_node, parser_pivot->rotation);

  res = darray_nodes_push_back(&solstice->pivots, &pivot_node);
  if(res != RES_OK) goto error;

  res = score_node_add_child(entity_node, pivot_node);
  if (res != RES_OK) goto error;

exit:
  *atchmnt_node = pivot_node;
  return entity_node;
error:
  if(pivot_node) {
    score_node_ref_put(pivot_node);
    pivot_node = NULL;
  }
  if(entity_node) {
    score_node_ref_put(entity_node);
    entity_node = NULL;
  }
  goto exit;
}

static struct score_node*
setup_entity(struct solstice* solstice, const struct solparser_entity* entity)
{
  struct score_node* root_node = NULL;
  struct score_node* atchmnt_node = NULL;
  struct score_node* tgt = NULL;
  struct score_node* child_root = NULL;
  size_t i;
  res_T res = RES_OK;
  ASSERT(solstice && solstice->parser && solstice->score && entity);

  switch(entity->type) {
    case SOLPARSER_ENTITY_EMPTY:
      atchmnt_node = root_node = setup_entity_empty(solstice, entity);
      break;
    case SOLPARSER_ENTITY_GEOMETRY:
      atchmnt_node = root_node = setup_entity_geometry(solstice, entity);
      break;
    case SOLPARSER_ENTITY_PIVOT:
      root_node = setup_entity_pivot(solstice, entity, &atchmnt_node);
      break;
    default: FATAL("Unreachable code.\n"); break;
  }

  if(!root_node || !atchmnt_node) {
    fprintf(stderr, "Could not setup the entity node.\n");
    goto error;
  }

  /* Register anchors */
  FOR_EACH(i, 0, solparser_entity_get_anchors_count(entity)) {
    struct solparser_anchor_id id;
    const struct solparser_anchor* anchor = NULL;

    res = score_node_tracking_target_create(solstice->score, &tgt);
    if(res != RES_OK) goto error;

    id = solparser_entity_get_anchor(entity, i);
    anchor = solparser_get_anchor(solstice->parser, id);

    res = htable_anchor_set(&solstice->anchors, &id.i, &tgt);
    if(res != RES_OK) goto error;

    score_node_set_translation(tgt, anchor->position);
    res = score_node_add_child(atchmnt_node, tgt);
    if(res != RES_OK) goto error;

    tgt = NULL;
  }

  /* Setup children */
  FOR_EACH(i, 0, solparser_entity_get_children_count(entity)) {
    struct solparser_entity_id id;
    const struct solparser_entity* child;

    id = solparser_entity_get_child(entity, i);
    child = solparser_get_entity(solstice->parser, id);

    child_root = setup_entity(solstice, child);
    if(!child_root) goto error;

    res = score_node_add_child(atchmnt_node, child_root);
    if(res != RES_OK) goto error;

    score_node_ref_put(child_root);
    child_root = NULL;
  }

exit:
  return root_node;
error:
  if(tgt) score_node_ref_put(tgt);
  if(child_root) score_node_ref_put(child_root);
  if(atchmnt_node && atchmnt_node!=root_node) score_node_ref_put(atchmnt_node);
  if(root_node) score_node_ref_put(root_node);
  atchmnt_node = NULL;
  root_node = NULL;
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_setup_entities(struct solstice* solstice)
{
  struct solparser_entity_iterator it, it_end;
  struct score_node* root = NULL;
  const double dummy_sun_dir[3] = {0, 0, 1}; /* Use the user defined dir */
  res_T res = RES_OK;
  ASSERT(solstice && solstice->parser && solstice->score);

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

    root = setup_entity(solstice, entity);
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
  if(root) score_node_ref_put(root);
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
    struct score_node* node = darray_nodes_data_get(&solstice->roots)[i];

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

