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

#ifndef SOLSTICE_CORE_NODE_H
#define SOLSTICE_CORE_NODE_H

#include <solstice/sanim.h>

#include <rsys/dynamic_array.h>
#include <rsys/ref_count.h>

struct sanim_node;

/* Define the darray_nodes data structure */
#define DARRAY_NAME nodes
#define DARRAY_DATA struct sanim_node*
#include <rsys/dynamic_array.h>

struct score_device;
struct score_node;
struct score_scene;
struct ssol_object;
struct ssol_instance;

enum node_type {
  NODE_TEMPLATE_ROOT,
  NODE_INSTANCE_ROOT,
  NODE_TEMPLATE,
  NODE_INSTANCE,
  NODE_TRACKING_TARGET,
  NODE_PIVOT,

  NODE_TYPES_COUNT__
};

struct instance_root_data {
  struct score_scene* scene_attachment;
  struct score_node* template;
  /* keep own pivots */
  struct darray_nodes pivots;
};

struct template_node_data {
  struct ssol_object* solver_object;
  int receiver_mask;
  int sample;
};

struct instance_node_data {
  struct ssol_instance* solver_instance;
  struct score_node* model;
  int receiver_mask;
  int sample;
};

struct score_node {
  enum node_type type;
  struct score_device* device;
  struct sanim_node anim;
  union {
    struct instance_root_data instance_root;
    struct template_node_data template_node;
    struct instance_node_data instance_node;
  } data;
  ref_T ref;
};

res_T
node_create
  (struct score_device* dev,
   struct score_node** out_node,
   enum node_type type);

void
node_ref_put_children(struct sanim_node* node);

res_T
node_instanciate_any
  (struct score_node* tp_node,
   struct score_node** out_node);

#endif /* SOLSTICE_CORE_NODE_H */
