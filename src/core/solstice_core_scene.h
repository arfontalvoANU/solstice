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

#ifndef score_scene_H
#define score_scene_H

#include <rsys/ref_count.h>

#include "solstice_core.h"

#ifndef SOLSTICE_DARRAY_NODES
#define SOLSTICE_DARRAY_NODES
#include <rsys/dynamic_array.h>
struct sanim_node;
/* Define the darray_nodes data structure */
#define DARRAY_NAME nodes
#define DARRAY_DATA struct sanim_node*
#include <rsys/dynamic_array.h>
#endif

struct ssol_scene;
struct ssol_sun;
struct ssol_atmosphere;

struct score_device;

struct score_scene {
  struct score_device* device;
  struct darray_nodes instances;
  struct ssol_scene* solver;
  struct ssol_sun* sun;
  struct ssol_atmosphere* atmosphere;
  ref_T ref;
};

#endif /* score_scene_H */
