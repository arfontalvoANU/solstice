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

#ifndef SOLSTICE_CORE_NODE_H
#define SOLSTICE_CORE_NODE_H

#include <solstice/sanim.h>

#include <rsys/dynamic_array.h>
#include <rsys/ref_count.h>

struct score_node;

/* Define the darray_nodes data structure */
#define DARRAY_NAME nodes
#define DARRAY_DATA struct score_node*
#include <rsys/dynamic_array.h>

struct score_device;
struct score_node;
struct score_scene;
struct ssol_object;
struct ssol_instance;

enum node_type {
  NODE_GEOMETRY,
  NODE_TRACKING_TARGET,
  NODE_PIVOT,
  NODE_EMPTY,

  NODE_TYPES_COUNT__
};

struct geometry_node_data {
  struct ssol_instance* solver_instance;
  int receiver_mask;
  int sample;
};

struct score_node {
  enum node_type type;
  struct score_device* device;
  struct sanim_node anim;
  union {
    /* only types of nodes with specific data */
    struct geometry_node_data geometry_node;
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

#endif /* SOLSTICE_CORE_NODE_H */
