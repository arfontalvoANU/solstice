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
solstice_setup_entity
  (struct solstice* solstice,
   const struct solparser_entity* entity,
   const char is_root,
   struct score_node** out_node)
{
  struct score_node* root_node = NULL;
  struct score_node* atchmnt_node = NULL;
  struct ssol_instance* instance = NULL;
  const struct solparser_anchor_id* anchors = NULL;
  const struct solparser_entity_id* children = NULL;
  size_t count, i;
  res_T res = RES_OK;
  ASSERT(solstice && solstice->parser && solstice->score && entity);

  /* TODO Split the cases un sub-functions! */
  switch (entity->type) {
  case SOLPARSER_ENTITY_EMPTY:
  {
    struct score_node* empty_node = NULL;
    res = score_node_empty_create(solstice->score, &empty_node);
    if (res != RES_OK) goto error;
    score_node_set_translation(empty_node, entity->translation);
    score_node_set_rotations(empty_node, entity->rotation);
    atchmnt_node = root_node = empty_node;
    break;
  }
  case SOLPARSER_ENTITY_GEOMETRY:
  {
    struct score_node* geometry_node = NULL;
    res = score_node_geometry_create(solstice->score, &geometry_node);
    if (res != RES_OK) goto error;
    score_node_set_translation(geometry_node, entity->translation);
    score_node_set_rotations(geometry_node, entity->rotation);
    res = solstice_instantiate_geometry
      (solstice, entity->data.geometry, &instance);
    if (res != RES_OK) goto error;
    res = score_node_geometry_setup(geometry_node, instance);
    if (res != RES_OK) goto error;
    atchmnt_node = root_node = geometry_node;
    break;
  }
  case SOLPARSER_ENTITY_PIVOT:
  {
    /* Each entity introduces a new coordinate system
     * and each object or pivot has some positionning in this system.
     * This behaviour is implemented through 2 levels of sanim_node. */
    /* TODO: remove pivot positionning? */
    struct score_node* entity_node = NULL;
    struct score_node* pivot_node = NULL;
    const struct solparser_pivot* parser_pivot = NULL;
    struct sanim_pivot anim_pivot = SANIM_PIVOT_NULL;
    struct sanim_tracking anim_tracking = SANIM_TRACKING_NULL;
    res = score_node_empty_create(solstice->score, &entity_node);
    if (res != RES_OK) goto error;
    score_node_set_translation(entity_node, entity->translation);
    score_node_set_rotations(entity_node, entity->rotation);
    res = score_node_pivot_create(solstice->score, &pivot_node);
    if (res != RES_OK) goto error;
    parser_pivot = solparser_get_pivot(solstice->parser, entity->data.pivot);
    ASSERT(parser_pivot);
    score_node_set_translation(pivot_node, parser_pivot->translation);
    score_node_set_rotations(pivot_node, parser_pivot->rotation);
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
    if (res != RES_OK) goto error;
    res = darray_nodes_push_back(&solstice->pivots, &pivot_node);
    if (res != RES_OK) goto error;
    root_node = entity_node;
    atchmnt_node = pivot_node;
    break;
  }
  default: FATAL("Unreachable code.\n"); break;
  }

  ASSERT(root_node && atchmnt_node);
  if (is_root) darray_nodes_push_back(&solstice->roots, &root_node);

  /* register anchors */
  count = darray_anchor_id_size_get(&entity->anchors);
  anchors = darray_anchor_id_cdata_get(&entity->anchors);
  for (i = 0; i < count; i++) {
    struct score_node* tgt;
    const struct solparser_anchor* anchor = NULL;
    res = score_node_tracking_target_create(solstice->score, &tgt);
    if (res != RES_OK) goto error;
    anchor = solparser_get_anchor(solstice->parser, anchors[i]);
    ASSERT(anchor);
    res = htable_anchor_set(&solstice->anchors, &anchors[i].i, &tgt);
    if (res != RES_OK) goto error;
    score_node_set_translation(tgt, anchor->position);
    res = score_node_add_child(atchmnt_node, tgt);
    if (res != RES_OK) goto error;
  }

  /* TODO: setup children */
  count = solparser_entity_get_children_count(entity);
  children = darray_child_id_cdata_get(&entity->children);
  for (i = 0; i < count; i++) {
    const struct solparser_entity* child;
    struct score_node* child_root = NULL;
    child = solparser_get_entity(solstice->parser, children[i]);
    ASSERT(child);
    res = solstice_setup_entity(solstice, child, 0, &child_root);
    if (res != RES_OK) goto error;
    ASSERT(child_root);
    res = score_node_add_child(atchmnt_node, child_root);
    if (res != RES_OK) goto error;
  }

end:
  if (instance) SSOL(instance_ref_put(instance));
  if (out_node) *out_node = root_node;
  return res;

error:
  goto end;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_setup_entities
  (struct solstice* solstice)
{
  struct solparser_entity_iterator it, it_end;
  res_T res = RES_OK;
  ASSERT(solstice && solstice->parser && solstice->score);

  /* Release possible previous roots (incomplete, TODO) */
  /*score_scene_clear(solstice->score);*/

  /* (re) create the list of roots from entities */
  solparser_entity_iterator_begin(solstice->parser, &it);
  solparser_entity_iterator_end(solstice->parser, &it_end);
  while (!solparser_entity_iterator_eq(&it, &it_end)) {
    struct solparser_entity_id entity_id;
    const struct solparser_entity* entity;

    entity_id = solparser_entity_iterator_get(&it);
    entity = solparser_get_entity(solstice->parser, entity_id);

    res = solstice_setup_entity(solstice, entity, 1, NULL);
    if (res != RES_OK) goto error;

    solparser_entity_iterator_next(&it);
  }

end:
  return res;
error:
  goto end;
}

