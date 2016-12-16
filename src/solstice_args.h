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

#ifndef SOLSTICE_ARGS_H
#define SOLSTICE_ARGS_H

#include <rsys/rsys.h>

struct solstice_args {
  const char* output_filename;
  unsigned long nrealisations; /* #realisations */

  struct {
    double pos[3];
    double tgt[3];
    double up[3];
    double fov_x;
  } camera;

  struct {
    unsigned long width;
    unsigned long height;
  } img;

  int rendering;
  int quiet;
};

#define SOLSTICE_ARGS_NULL__ {0}
static const struct solstice_args SOLSTICE_ARGS_NULL = SOLSTICE_ARGS_NULL__;

extern LOCAL_SYM res_T
solstice_args_init
  (struct solstice_args* args,
   const int argc,
   char** argv);

extern LOCAL_SYM void
solstice_args_release
  (struct solstice_args* args);

#endif /* SOLSTICE_ARGS_H */

