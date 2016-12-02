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

#ifndef TEST_CORE_UTILS_H
#define TEST_CORE_UTILS_H

#include "solstice_core.h"

#include <rsys/rsys.h>
#include <stdio.h>

struct mem_allocator;
struct ssol_device;
struct ssol_param_buffer;

/*******************************************************************************
 * Utilities
 ******************************************************************************/
void
log_stream(const char* msg, void* ctx);

/*******************************************************************************
 * Mesh stuff
 ******************************************************************************/
struct desc {
  const float* vertices;
  const unsigned* indices;
};

extern const float EDGES__ [];

extern const unsigned RECT_NVERTS__;

extern const unsigned TRG_IDS__ [];
extern const unsigned RECT_NTRIS__;

extern const struct desc RECT_DESC__;

void
get_position(const unsigned ivert, float position[3], void* data);

void
get_ids(const unsigned itri, unsigned ids[3], void* data);

void
get_polygon_vertices(const size_t ivert, double position[2], void* ctx);

void
get_shader_normal
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val);

void
get_shader_reflectivity
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val);

void
get_shader_roughness
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val);

res_T
pp_sum
  (FILE* f,
   const int32_t receiver_id,
   const size_t count,
   double* mean,
   double* std);

#endif /* TEST_CORE_UTILS_H */

