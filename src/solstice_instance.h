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

#ifndef SOLSTICE_INSTANCE_H
#define SOLSTICE_INSTANCE_H

#include "solstice_node.h"
#include "solstice_shape.h"

enum solstice_instance_type {
  SOLSTICE_INSTANCE_OBJECT,
  SOLSTICE_INSTANCE_TREE
};

struct solstice_instance {
  double rotation[3];
  double translation[3];
  enum solstice_instance_type type;
  union {
    struct solstice_object_id object;
    struct solstice_node_id tree;
  } data;
};

struct solstice_instance_id { size_t i; };

#endif /* SOLSTICE_INSTANCE_H */

