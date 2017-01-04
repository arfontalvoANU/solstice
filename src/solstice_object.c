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

#include <rsys/double33.h>
#include <rsys/float3.h>
#include <solstice/ssol.h>
#include <star/s3dut.h>

#include <limits.h>

#define MAX_POLYCLIPS 16

struct mesh_context {
  struct s3dut_mesh_data data;
  double transform[12];
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
get_ids(const unsigned itri, unsigned ids[3], void* ctx)
{
  const struct mesh_context* mesh = ctx;
  ASSERT(ids && ctx && itri < mesh->data.nprimitives);
  ASSERT(mesh->data.indices[itri*3+0] <= UINT_MAX);
  ASSERT(mesh->data.indices[itri*3+1] <= UINT_MAX);
  ASSERT(mesh->data.indices[itri*3+2] <= UINT_MAX);
  ids[0] = (unsigned)mesh->data.indices[itri*3+0];
  ids[1] = (unsigned)mesh->data.indices[itri*3+1];
  ids[2] = (unsigned)mesh->data.indices[itri*3+2];
}

static void
get_pos(const unsigned ivert, float pos[3], void* ctx)
{
  const struct mesh_context* mesh = ctx;
  double tmp[3];
  ASSERT(pos && ctx && ivert < mesh->data.nvertices);
  d3_set(tmp, mesh->data.positions + ivert*3);
  d33_muld3(tmp, mesh->transform, tmp);
  d3_add(tmp, mesh->transform+9, tmp);
  f3_set_d3(pos, tmp);
}

static void
get_carving_pos(const size_t ivert, double pos[2], void* ctx)
{
  const struct solparser_polyclip* polyclip = ctx;
  ASSERT(pos && ctx);
  solparser_polyclip_get_vertex(polyclip, ivert, pos);
}

static INLINE enum ssol_clipping_op
solparser_clip_op_to_ssol_clipping_op(const enum solparser_clip_op op)
{
  switch(op) {
    case SOLPARSER_CLIP_OP_SUB: return SSOL_SUB;
    case SOLPARSER_CLIP_OP_AND: return SSOL_AND;
    default: FATAL("Unreachable code.\n");
  }
}

static res_T
create_ssol_shape_mesh
  (struct solstice* solstice,
   const double transform[12],
   const struct s3dut_mesh* mesh,
   struct ssol_shape** out_ssol_shape)
{
  struct mesh_context mesh_ctx;
  struct ssol_vertex_data vertex_data = SSOL_VERTEX_DATA_NULL;
  struct ssol_shape* ssol_shape = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && mesh && out_ssol_shape);

  S3DUT(mesh_get_data(mesh, &mesh_ctx.data));
  ASSERT(mesh_ctx.data.nprimitives <= UINT_MAX);
  ASSERT(mesh_ctx.data.nvertices <= UINT_MAX);
  d33_set(mesh_ctx.transform, transform);
  d3_set(mesh_ctx.transform+9, transform+9);

  res = ssol_shape_create_mesh(solstice->ssol, &ssol_shape);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create a Solstice Solver mesh shape.\n");
    goto error;
  }

  vertex_data.usage = SSOL_POSITION;
  vertex_data.get = get_pos;
  res = ssol_mesh_setup(ssol_shape, (unsigned)mesh_ctx.data.nprimitives,
    get_ids, (unsigned)mesh_ctx.data.nvertices, &vertex_data, 1, &mesh_ctx);
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
   const double transform[12],
   const struct solparser_shape_cuboid_id cuboid_id,
   struct ssol_shape** out_ssol_shape)
{
  const struct solparser_shape_cuboid* cuboid;
  struct s3dut_mesh* mesh = NULL;
  struct ssol_shape* ssol_shape = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && out_ssol_shape);

  cuboid = solparser_get_shape_cuboid(solstice->parser, cuboid_id);
  res = s3dut_create_cuboid(solstice->allocator, cuboid->size[0],
    cuboid->size[1], cuboid->size[2], &mesh);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the cuboid 3D data.\n");
    goto error;
  }

  res = create_ssol_shape_mesh(solstice, transform, mesh, &ssol_shape);
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
create_cylinder
  (struct solstice* solstice,
   const double transform[12],
   const struct solparser_shape_cylinder_id cylinder_id,
   struct ssol_shape** out_ssol_shape)
{
  const struct solparser_shape_cylinder* cylinder;
  struct s3dut_mesh* mesh = NULL;
  struct ssol_shape* ssol_shape = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && out_ssol_shape);

  cylinder = solparser_get_shape_cylinder(solstice->parser, cylinder_id);
  ASSERT(cylinder->nslices > 0 && cylinder->nslices < UINT_MAX);
  res = s3dut_create_cylinder(solstice->allocator, cylinder->radius,
    cylinder->height, (unsigned)cylinder->nslices, 1, &mesh);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the cylinder 3D data.\n");
    goto error;
  }

  res = create_ssol_shape_mesh(solstice, transform, mesh, &ssol_shape);
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
create_sphere
  (struct solstice* solstice,
   const double transform[12],
   const struct solparser_shape_sphere_id sphere_id,
   struct ssol_shape** out_ssol_shape)
{
  const struct solparser_shape_sphere* sphere;
  struct s3dut_mesh* mesh = NULL;
  struct ssol_shape* ssol_shape = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && out_ssol_shape);

  sphere = solparser_get_shape_sphere(solstice->parser, sphere_id);
  ASSERT(sphere->nslices > 0 && sphere->nslices < UINT_MAX);
  res = s3dut_create_sphere(solstice->allocator, sphere->radius,
    (unsigned)sphere->nslices, (unsigned)(sphere->nslices/2), &mesh);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the sphere 3D data.\n");
    goto error;
  }

  res = create_ssol_shape_mesh(solstice, transform, mesh, &ssol_shape);
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
create_ssol_shape_punched_surface
  (struct solstice* solstice,
   const struct darray_polyclip* clips,
   struct ssol_quadric* quadric,
   struct ssol_shape** out_ssol_shape)
{
  struct ssol_shape* ssol_shape = NULL;
  struct ssol_carving carvings[MAX_POLYCLIPS];
  struct ssol_punched_surface punched_surf = SSOL_PUNCHED_SURFACE_NULL;
  size_t iclip, nclips;
  res_T res = RES_OK;
  ASSERT(solstice && quadric && out_ssol_shape);

  nclips = darray_polyclip_size_get(clips);
  if(nclips > MAX_POLYCLIPS) {
    fprintf(stderr, "Too many clipping polygons. "
"%lu are submitted while at most %lu can be defined.\n",
      (unsigned long)nclips, (unsigned long)MAX_POLYCLIPS);
    res = RES_BAD_ARG;
    goto error;
  }

  FOR_EACH(iclip, 0, nclips) {
    const struct solparser_polyclip* clip;
    clip = darray_polyclip_cdata_get(clips) + iclip;

    carvings[iclip].get = get_carving_pos;
    carvings[iclip].nb_vertices = solparser_polyclip_get_vertices_count(clip);
    carvings[iclip].operation = solparser_clip_op_to_ssol_clipping_op(clip->op);
    carvings[iclip].context = (void*)clip;
  }

  punched_surf.quadric = quadric;
  punched_surf.carvings = carvings;
  punched_surf.nb_carvings = nclips;

  res = ssol_shape_create_punched_surface(solstice->ssol, &ssol_shape);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create a Solstice Solver punched surface.\n");
    goto error;
  }

  res = ssol_punched_surface_setup(ssol_shape, &punched_surf);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the Solstice Solver punched surface.\n");
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
create_parabol
  (struct solstice* solstice,
   const double transform[12],
   const struct solparser_shape_paraboloid_id id,
   struct ssol_shape** out_ssol_shape)
{
  const struct solparser_shape_paraboloid* paraboloid;
  struct ssol_quadric quadric = SSOL_QUADRIC_DEFAULT;
  ASSERT(solstice);

  paraboloid = solparser_get_shape_parabol(solstice->parser, id);

  quadric.type = SSOL_QUADRIC_PARABOL;
  quadric.data.parabol.focal = paraboloid->focal;
  d33_set(quadric.transform, transform);
  d3_set(quadric.transform+9, transform+9);

  return create_ssol_shape_punched_surface
    (solstice, &paraboloid->polyclips, &quadric, out_ssol_shape);
}

static res_T
create_parabolic_cylinder
  (struct solstice* solstice,
   const double transform[12],
   const struct solparser_shape_paraboloid_id id,
   struct ssol_shape** out_ssol_shape)
{
  const struct solparser_shape_paraboloid* paraboloid;
  struct ssol_quadric quadric = SSOL_QUADRIC_DEFAULT;
  ASSERT(solstice);

  paraboloid = solparser_get_shape_parabolic_cylinder(solstice->parser, id);

  quadric.type = SSOL_QUADRIC_PARABOLIC_CYLINDER;
  quadric.data.parabol.focal = paraboloid->focal;
  d33_set(quadric.transform, transform);
  d3_set(quadric.transform+9, transform+9);

  return create_ssol_shape_punched_surface
    (solstice, &paraboloid->polyclips, &quadric, out_ssol_shape);

}

static res_T
create_plane
  (struct solstice* solstice,
   const double transform[12],
   const struct solparser_shape_plane_id id,
   struct ssol_shape** out_ssol_shape)
{
  const struct solparser_shape_plane* plane;
  struct ssol_quadric quadric;
  ASSERT(solstice);

  plane = solparser_get_shape_plane(solstice->parser, id);

  quadric.type = SSOL_QUADRIC_PLANE;
  d33_set(quadric.transform, transform);
  d3_set(quadric.transform+9, transform+9);

  return create_ssol_shape_punched_surface
    (solstice, &plane->polyclips, &quadric, out_ssol_shape);
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
  double transform[12];
  res_T res = RES_OK;
  ASSERT(solstice && ssol_front && ssol_back && ssol_shape);

  obj = solparser_get_object(solstice->parser, obj_id);

  mtl2 = solparser_get_material_double_sided(solstice->parser, obj->mtl2);
  solstice_get_ssol_material(solstice, mtl2->front, ssol_front);
  solstice_get_ssol_material(solstice, mtl2->back, ssol_back);

  /* Define the shape transformation */
  d33_rotation(transform, obj->rotation[0], obj->rotation[1], obj->rotation[2]);
  d33_muld3(transform+9, transform, obj->translation);

  shape = solparser_get_shape(solstice->parser, obj->shape);
  switch(shape->type) {
    case SOLPARSER_SHAPE_CUBOID:
      res = create_cuboid(solstice, transform, shape->data.cuboid, ssol_shape);
      break;
    case SOLPARSER_SHAPE_CYLINDER:
      res = create_cylinder
        (solstice, transform, shape->data.cylinder, ssol_shape);
      break;
    case SOLPARSER_SHAPE_OBJ:
      fprintf(stderr, "`obj' shapes are not supported yet.\n");
      res = RES_BAD_ARG;
      break;
    case SOLPARSER_SHAPE_PARABOL:
      res = create_parabol(solstice, transform, shape->data.parabol, ssol_shape);
      break;
    case SOLPARSER_SHAPE_PARABOLIC_CYLINDER:
      res = create_parabolic_cylinder
        (solstice, transform, shape->data.parabol, ssol_shape);
      break;
    case SOLPARSER_SHAPE_PLANE:
      res = create_plane(solstice, transform, shape->data.plane, ssol_shape);
      break;
    case SOLPARSER_SHAPE_SPHERE:
      res = create_sphere(solstice, transform, shape->data.sphere, ssol_shape);
      break;
    case SOLPARSER_SHAPE_STL:
      fprintf(stderr, "`STL' shapes are not supported yet.\n");
      res = RES_BAD_ARG;
      break;
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

