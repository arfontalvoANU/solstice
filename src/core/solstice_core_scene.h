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

#ifndef SCORE_SCENE_H
#define SCORE_SCENE_H

#include "solstice_core.h"
#include "solstice_core_node.h"

#include <rsys/dynamic_array.h>
#include <rsys/ref_count.h>

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

#endif /* SCORE_SCENE_H */
