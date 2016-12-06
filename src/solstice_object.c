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

#include "solstice_c.h"

#include <solstice/ssol.h>
#include <star/s3dut.h>

#include <limits.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
get_ids(const unsigned itri, unsigned ids[3], void* ctx)
{
  const struct s3dut_mesh_data* mesh = ctx;
  ASSERT(ids && ctx && itri < mesh->nprimitives);
  ASSERT(mesh->indices[itri*3+0] <= UINT_MAX);
  ASSERT(mesh->indices[itri*3+1] <= UINT_MAX);
  ASSERT(mesh->indices[itri*3+2] <= UINT_MAX);
  ids[0] = (unsigned)mesh->indices[itri*3+0];
  ids[1] = (unsigned)mesh->indices[itri*3+1];
  ids[2] = (unsigned)mesh->indices[itri*3+2];
}

static void
get_pos(const unsigned ivert, float pos[3], void* ctx)
{
  const struct s3dut_mesh_data* mesh = ctx;
  ASSERT(pos && ctx && ivert < mesh->nvertices);
  pos[0] = (float)mesh->positions[ivert*3+0];
  pos[1] = (float)mesh->positions[ivert*3+1];
  pos[2] = (float)mesh->positions[ivert*3+2];
}

static res_T
create_ssol_shape_mesh
  (struct solstice* solstice,
   const struct s3dut_mesh* mesh,
   struct ssol_shape** out_ssol_shape)
{
  struct s3dut_mesh_data mesh_data;
  struct ssol_vertex_data vertex_data = SSOL_VERTEX_DATA_NULL;
  struct ssol_shape* ssol_shape = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && mesh && out_ssol_shape);

  S3DUT(mesh_get_data(mesh, &mesh_data));
  ASSERT(mesh_data.nprimitives <= UINT_MAX);
  ASSERT(mesh_data.nvertices <= UINT_MAX);

  res = ssol_shape_create_mesh(solstice->ssol, &ssol_shape);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create a Solstice Solver mesh shape.\n");
    goto error;
  }

  vertex_data.usage = SSOL_POSITION;
  vertex_data.get = get_pos;
  res = ssol_mesh_setup(ssol_shape, (unsigned)mesh_data.nprimitives, get_ids,
    (unsigned)mesh_data.nvertices, &vertex_data, 1, &mesh_data);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup a Solstice Solver mesh shape.\n");
    goto error;
  }

exit:
  *out_ssol_shape = ssol_shape;
  return res;
error:
  if(ssol_shape) {
    SSOL(shape_ref_put(ssol_shape));
    ssol_shape = NULL;
  }
  goto exit;
}

static res_T
create_cuboid
  (struct solstice* solstice,
   const struct solparser_shape_cuboid_id cuboid_id,
   struct ssol_shape** out_ssol_shape)
{
  const struct solparser_shape_cuboid* cuboid;
  struct s3dut_mesh* mesh = NULL;
  struct ssol_shape* ssol_shape = NULL;
  res_T res = RES_OK;
  ASSERT(out_ssol_shape);

  cuboid = solparser_get_shape_cuboid(solstice->parser, cuboid_id);
  res = s3dut_create_cuboid(solstice->allocator, cuboid->size[0],
    cuboid->size[1], cuboid->size[2], &mesh);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the cuboid 3D data.\n");
    goto error;
  }

  res = create_ssol_shape_mesh(solstice, mesh, &ssol_shape);
  if(res != RES_OK) goto error;

exit:
  if(mesh) S3DUT(mesh_ref_put(mesh));
  *out_ssol_shape = ssol_shape;
  return res;
error:
  if(ssol_shape) {
    SSOL(shape_ref_put(ssol_shape));
    ssol_shape = NULL;
  }
  goto exit;
}

static res_T
create_shaded_shape
  (struct solstice* solstice,
   const struct solparser_object_id obj_id,
   struct ssol_material** ssol_front,
   struct ssol_material** ssol_back,
   struct ssol_shape** ssol_shape)
{
  const struct solparser_material_double_sided* mtl2;
  const struct solparser_object* obj;
  const struct solparser_shape* shape;
  res_T res = RES_OK;
  ASSERT(solstice && ssol_front && ssol_back && ssol_shape);

  obj = solparser_get_object(solstice->parser, obj_id);

  mtl2 = solparser_get_material_double_sided(solstice->parser, obj->mtl2);
  solstice_get_ssol_material(solstice, mtl2->front, ssol_front);
  solstice_get_ssol_material(solstice, mtl2->back, ssol_back);

  shape = solparser_get_shape(solstice->parser, obj->shape);
  switch(shape->type) {
    case SOLPARSER_SHAPE_CUBOID:
      res = create_cuboid(solstice, shape->data.cuboid, ssol_shape);
      break;
    case SOLPARSER_SHAPE_CYLINDER: break;
    case SOLPARSER_SHAPE_OBJ: break;
    case SOLPARSER_SHAPE_PARABOL: break;
    case SOLPARSER_SHAPE_PARABOLIC_CYLINDER: break;
    case SOLPARSER_SHAPE_PLANE: break;
    case SOLPARSER_SHAPE_SPHERE: break;
    case SOLPARSER_SHAPE_STL: break;
    default: FATAL("Unreachable code.\n"); break;
  }
  return res;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_instantiate_geometry
  (struct solstice* solstice,
   const struct solparser_geometry_id geom_id,
   struct ssol_instance** out_inst)
{
  struct ssol_instance* inst = NULL;
  struct ssol_object** pssol_obj = NULL;
  struct ssol_object* ssol_obj = NULL;
  struct ssol_object* ssol_obj_new = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && out_inst);

  pssol_obj = htable_object_find(&solstice->objects, &geom_id.i);
  if(pssol_obj) {
    ssol_obj = *pssol_obj;
  } else {
    const struct solparser_geometry* geom;
    size_t iobj, nobjs;

    res = ssol_object_create(solstice->ssol, &ssol_obj_new);
    if(res != RES_OK) {
      fprintf(stderr, "Could not create a Solstice Solver object.\n");
      goto error;
    }
    ssol_obj = ssol_obj_new;

    geom = solparser_get_geometry(solstice->parser, geom_id);
    nobjs = solparser_geometry_get_objects_count(geom);
    FOR_EACH(iobj, 0, nobjs) {
      struct solparser_object_id obj_id;
      struct ssol_shape* shape = NULL;
      struct ssol_material* front;
      struct ssol_material* back;

      obj_id = solparser_geometry_get_object(geom, iobj);
      res = create_shaded_shape(solstice, obj_id, &front, &back, &shape);
      if(res != RES_OK) goto error;

      res = ssol_object_add_shaded_shape(ssol_obj, shape, front, back);
      if(res != RES_OK) {
        fprintf(stderr,
          "Could not add a shaded shape to a Solstice Solver object.\n");
        goto error;
      }
    }
  }

  res = ssol_object_instantiate(ssol_obj, &inst);
  if(res != RES_OK) {
    fprintf(stderr, "Could not instantiate a Solstice Solver object.\n");
    goto error;
  }

  res = htable_object_set(&solstice->objects, &geom_id.i, &ssol_obj);
  if(res != RES_OK) {
    fprintf(stderr, "Could not register a Solstice Solver object.\n");
    goto error;
  }

exit:
  *out_inst = inst;
  return res;
error:
  if(inst) SSOL(instance_ref_put(inst)), inst = NULL;
  if(ssol_obj_new) SSOL(object_ref_put(ssol_obj_new));
  goto exit;
}

