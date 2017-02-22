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

#include "solstice_c.h"
#include <solstice/ssol.h>

struct dump_context {
  FILE* output;
  size_t ids_offset;
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
dump_instantiated_shaded_shape
  (struct ssol_instantiated_shaded_shape* sshape, struct dump_context* ctx)
{
  unsigned i, ntris, nverts;
  ASSERT(sshape && ctx);

  SSOL(shape_get_vertices_count(sshape->shape, &nverts));
  FOR_EACH(i, 0, nverts) {
    double pos[3];
    SSOL(instantiated_shaded_shape_get_vertex_attrib
      (sshape, i, SSOL_POSITION, pos));
    fprintf(ctx->output, "v %g %g %g\n", SPLIT3(pos));
  }

  SSOL(shape_get_triangles_count(sshape->shape, &ntris));
  FOR_EACH(i, 0, ntris) {
    unsigned ids[3];
    SSOL(shape_get_triangle_indices(sshape->shape, i, ids));
    /* Note that in the obj fileformat the first index is 1 rather than 0 */
    fprintf(ctx->output, "f %lu %lu %lu\n",
      (unsigned long)(ids[0] + 1 + ctx->ids_offset),
      (unsigned long)(ids[1] + 1 + ctx->ids_offset),
      (unsigned long)(ids[2] + 1 + ctx->ids_offset));
  }

  ctx->ids_offset += nverts;
}

static res_T
dump_instance(struct ssol_instance* instance, void* context)
{
  struct dump_context* ctx = context;
  size_t i, n;
  ASSERT(instance && ctx);

  SSOL(instance_get_shaded_shapes_count(instance, &n));
  FOR_EACH(i, 0, n) {
    struct ssol_instantiated_shaded_shape sshape;

    SSOL(instance_get_shaded_shape(instance, i, &sshape));
    dump_instantiated_shaded_shape(&sshape, ctx);
  }

  return RES_OK;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_dump_obj(struct solstice* solstice)
{
  struct dump_context ctx;
  res_T res = RES_OK;
  ASSERT(solstice);

  ctx.output = solstice->output;
  ctx.ids_offset = 0;

  res = ssol_scene_for_each_instance(solstice->scene, dump_instance, &ctx);
  if(res != RES_OK) {
    fprintf(stderr, "Could not dump the solstice geometry.\n");
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

