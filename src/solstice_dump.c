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
  enum solstice_args_dump_split_mode split_mode;
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
dump_instantiated_shaded_shape
  (struct ssol_instantiated_shaded_shape* sshape, struct dump_context* ctx)
{
  unsigned i, ntris, nverts;
  enum ssol_material_type type;
  const char* mtl;
  ASSERT(sshape && ctx);

  SSOL(material_get_type(sshape->mtl_front, &type));
  switch(type) {
    case SSOL_MATERIAL_DIELECTRIC: mtl = "dielectric"; break;
    case SSOL_MATERIAL_MATTE: mtl = "matte"; break;
    case SSOL_MATERIAL_MIRROR: mtl = "mirror"; break;
    case SSOL_MATERIAL_THIN_DIELECTRIC: mtl = "thin_dielectric"; break;
    case SSOL_MATERIAL_VIRTUAL: mtl = "virtual"; break;
    default: FATAL("Unexpected Solstice Solver material type.\n"); break;
  }

  fprintf(ctx->output, "usemtl %s\n", mtl);

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

  if(ctx->split_mode == SOLSTICE_ARGS_DUMP_SPLIT_OBJECT) {
    fprintf(ctx->output, "---\n");
    ctx->ids_offset = 0;
  } else {
    ctx->ids_offset += nverts;
  }
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

static res_T
dump_geometry
  (const struct sanim_node* n, const double transform[12], void* data)
{
  struct solstice_node* node;
  struct dump_context* ctx = data;
  res_T res = RES_OK;
  ASSERT(n && data);
  (void)transform;

  node = CONTAINER_OF(n, struct solstice_node, anim);
  if(node->type != SOLSTICE_NODE_GEOMETRY) return RES_OK;
  fprintf(ctx->output, "g %s\n", solstice_node_get_name(node));
  res = dump_instance(node->instance, data);
  if(res != RES_OK) return res;

  if(ctx->split_mode == SOLSTICE_ARGS_DUMP_SPLIT_GEOMETRY) {
    fprintf(ctx->output, "---\n");
    ctx->ids_offset = 0;
  }
  return RES_OK;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_dump(struct solstice* solstice)
{
  struct dump_context ctx;
  size_t i, n;
  res_T res = RES_OK;
  ASSERT(solstice && solstice->dump_format == SOLSTICE_ARGS_DUMP_OBJ);

  ctx.output = solstice->output;
  ctx.ids_offset = 0;
  ctx.split_mode = solstice->dump_split_mode;

  n = darray_nodes_size_get(&solstice->roots);
  FOR_EACH(i, 0, n) {
    struct solstice_node* node = darray_nodes_data_get(&solstice->roots)[i];

    fprintf(solstice->output, "# %s\n", solstice_node_get_name(node));

    res = sanim_node_visit_tree(&node->anim, NULL, &ctx, dump_geometry);
    if(res != RES_OK) {
      fprintf(stderr, "Could not dump the solstice geometry.\n");
      goto error;
    }
  }

exit:
  return res;
error:
  goto exit;
}

