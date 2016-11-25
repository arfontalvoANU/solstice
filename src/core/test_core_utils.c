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

#include "test_core_utils.h"

#include <solstice/ssol.h>

#include <rsys/mem_allocator.h>
#include <rsys/logger.h>

#define _POSIX_C_SOURCE 200809L /* snprintf support */
#include <math.h>
#include <stdio.h>
#include <string.h>

void
log_stream(const char* msg, void* ctx) {
  ASSERT(msg);
  (void) msg, (void) ctx;
  printf("%s\n", msg);
}

void
check_memory_allocator(struct mem_allocator* allocator) {
  if (MEM_ALLOCATED_SIZE(allocator)) {
    char dump[512];
    MEM_DUMP(allocator, dump, sizeof(dump) / sizeof(char));
    fprintf(stderr, "%s\n", dump);
    FATAL("Memory leaks\n");
  }
}

const float VERTICES__ [] = {
  -1, -1, 0.f,
  1, -1, 0.f,
  1,  1, 0.f,
  -1,  1, 0.f
};

const unsigned RECT_NVERTS__ = sizeof(VERTICES__) / sizeof(float[3]);

const unsigned TRG_IDS__ [] = { 0, 2, 1, 2, 0, 3 };
const unsigned RECT_NTRIS__ = sizeof(TRG_IDS__) / sizeof(unsigned[3]);

const struct desc RECT_DESC__ = { VERTICES__, TRG_IDS__ };

void
get_position(const unsigned ivert, float position[3], void* data)
{
  struct desc* desc = data;
  NCHECK(desc, NULL);
  NCHECK(position, NULL);
  position[0] = desc->vertices[ivert * 3 + 0];
  position[1] = desc->vertices[ivert * 3 + 1];
  position[2] = desc->vertices[ivert * 3 + 2];
}

void
get_ids(const unsigned itri, unsigned ids[3], void* data)
{
  const unsigned id = itri * 3;
  struct desc* desc = data;
  NCHECK(desc, NULL);
  NCHECK(ids, NULL);
  ids[0] = desc->indices[id + 0];
  ids[1] = desc->indices[id + 1];
  ids[2] = desc->indices[id + 2];
}

void
get_polygon_vertices(const size_t ivert, double position[2], void* ctx)
{
  const double* verts = ctx;
  NCHECK(position, NULL);
  NCHECK(ctx, NULL);
  position[0] = verts[ivert * 2 + 0];
  position[1] = verts[ivert * 2 + 1];
}

void
get_shader_normal
  (struct ssol_device* dev,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  int i;
  (void) dev, (void) wavelength, (void) P, (void) Ng, (void) uv, (void) w;
  FOR_EACH(i, 0, 3) val[i] = Ns[i];
}

void
get_shader_reflectivity
  (struct ssol_device* dev,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  (void) dev, (void) wavelength, (void) P, (void) Ng, (void) Ns, (void) uv, (void) w;
  *val = 1;
}

void
get_shader_roughness
  (struct ssol_device* dev,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  (void) dev, (void) wavelength, (void) P, (void) Ng, (void) Ns, (void) uv, (void) w;
  *val = 0;
}

res_T
pp_sum
  (FILE* f,
   const int32_t receiver_id,
   const size_t count,
   double* mean,
   double* std)
{
  struct ssol_receiver_data hit;
  double sum = 0;
  double sum2 = 0;
  double E, V, SE;

  if (!f || !mean || !std || !count)
    return RES_BAD_ARG;

  rewind(f);
  while (1 == fread(&hit, sizeof(struct ssol_receiver_data), 1, f)) {
    if (ferror(f))
      return RES_BAD_ARG;

    if (receiver_id != hit.receiver_id)
      continue;

    sum += hit.weight;
    sum2 += hit.weight * hit.weight;
  }

  E = sum / (double) count;
  V = MMAX(sum2 / (double) count - E*E, 0);
  SE = sqrt(V / (double) count);

  *mean = E;
  *std = SE;
  return RES_OK;
}
