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

#include "solstice_core.h"
#include "test_core_utils.h"

#include <solstice/ssol.h>
#include <solstice/sanim.h>

#include <rsys/logger.h>
#include <rsys/double3.h>

int
main(int argc, char** argv)
{
  struct logger logger;
  struct mem_allocator allocator;
  struct score_device* dev = NULL;
  struct score_node *temp = NULL, *inst = NULL, *piv = NULL, *piv2 = NULL,
    *geom1 = NULL, *geom2 = NULL, *tgt = NULL;
  struct sanim_pivot pivot = SANIM_PIVOT_NULL;
  struct sanim_tracking tracking = SANIM_TRACKING_NULL;
  struct ssol_device* sol_dev = NULL;
  struct ssol_punched_surface punched = SSOL_PUNCHED_SURFACE_NULL;
  struct ssol_carving carving = SSOL_CARVING_NULL;
  struct ssol_quadric quadric = SSOL_QUADRIC_DEFAULT;
  struct ssol_shape *shape1 = NULL, *shape2 = NULL;
  struct ssol_material* mtl = NULL;
  struct ssol_object *obj1 = NULL, *obj2 = NULL;
  struct ssol_vertex_data attribs[1] = { SSOL_VERTEX_DATA_NULL__ };
  double polygon [] = {
    -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 0.f, -2.f
  };
  double transl[3], rot[3];
  const size_t npolygon_verts = sizeof(polygon) / sizeof(double[2]);
  (void) argc, (void) argv;

  mem_init_proxy_allocator(&allocator, &mem_default_allocator);

  CHECK(logger_init(&allocator, &logger), RES_OK);
  logger_set_stream(&logger, LOG_OUTPUT, log_stream, NULL);
  logger_set_stream(&logger, LOG_ERROR, log_stream, NULL);
  logger_set_stream(&logger, LOG_WARNING, log_stream, NULL);

  CHECK(ssol_device_create(
    &logger, &allocator, SSOL_NTHREADS_DEFAULT, 0, &sol_dev), RES_OK);

  score_device_create(&logger, &allocator, 1, 0, &dev);

  CHECK(score_node_template_create(NULL, &temp), RES_BAD_ARG);
  CHECK(score_node_template_create(dev, NULL), RES_BAD_ARG);
  CHECK(score_node_template_create(dev, &temp), RES_OK);
  score_node_ref_put(temp);

  CHECK(score_node_create_object(NULL, &geom1), RES_BAD_ARG);
  CHECK(score_node_create_object(dev, NULL), RES_BAD_ARG);
  CHECK(score_node_create_object(dev, &geom1), RES_OK);
  score_node_ref_put(geom1);
  
  CHECK(score_node_pivot_create(NULL, &piv), RES_BAD_ARG);
  CHECK(score_node_pivot_create(dev, NULL), RES_BAD_ARG);
  CHECK(score_node_pivot_create(dev, &piv), RES_OK);
  score_node_ref_put(piv);

  CHECK(score_node_tracking_target_create(NULL, &tgt), RES_BAD_ARG);
  CHECK(score_node_tracking_target_create(dev, NULL), RES_BAD_ARG);
  CHECK(score_node_tracking_target_create(dev, &tgt), RES_OK);
  score_node_ref_put(tgt);

  CHECK(score_node_template_create(dev, &temp), RES_OK);
  CHECK(score_node_create_object(dev, &geom1), RES_OK);
  CHECK(score_node_create_object(dev, &geom2), RES_OK);
  CHECK(score_node_pivot_create(dev, &piv), RES_OK);
  CHECK(score_node_pivot_create(dev, &piv2), RES_OK);
  CHECK(score_node_tracking_target_create(dev, &tgt), RES_OK);

  tracking.data.node_target.tracked_node = NULL;
  tracking.policy = TRACKING_NODE_TARGET;
  score_node_track_me(tgt, &tracking);
  CHECK(tracking.policy, TRACKING_NODE_TARGET);
  /* cannot check tracking.data.node_target.tracked_node validity */

  CHECK(score_node_instantiate(NULL, &inst), RES_BAD_ARG);
  CHECK(score_node_instantiate(temp, NULL), RES_BAD_ARG);
  CHECK(score_node_instantiate(geom1, &inst), RES_BAD_ARG);
  CHECK(score_node_instantiate(temp, &inst), RES_OK);
  score_node_ref_put(inst);

  CHECK(score_node_instantiate(temp, &inst), RES_OK);

  score_node_set_receiver(geom1, 0);
  score_node_set_receiver(geom2, 0);

  score_node_sample(geom1, 0);
  score_node_sample(geom2, 1);

  CHECK(ssol_material_create_virtual(sol_dev, &mtl), RES_OK);
  CHECK(ssol_object_create(sol_dev, &obj1), RES_OK);
  CHECK(ssol_object_create(sol_dev, &obj2), RES_OK);

  attribs[0].usage = SSOL_POSITION;
  attribs[0].get = get_position;
  CHECK(ssol_shape_create_mesh(sol_dev, &shape1), RES_OK);
  CHECK(ssol_mesh_setup(shape1, RECT_NTRIS__, get_ids, RECT_NVERTS__,
    attribs, 1, (void*) &RECT_DESC__), RES_OK);
  CHECK(ssol_object_add_shaded_shape(obj1, shape1, mtl, mtl), RES_OK);

  tracking.policy = TRACKING_SUN;
  pivot.type = PIVOT_SINGLE_AXIS;
  d3(pivot.data.pivot1.ref_normal, 0, 0, 1);
  CHECK(score_node_object_setup(NULL, obj1), RES_BAD_ARG);
  CHECK(score_node_object_setup(geom1, NULL), RES_BAD_ARG);
  CHECK(score_node_object_setup(geom1, obj1), RES_OK);

  CHECK(score_node_pivot_setup(piv, &pivot, &tracking), RES_OK);
  CHECK(score_node_pivot_setup(piv2, &pivot, &tracking), RES_OK);

  carving.get = get_polygon_vertices;
  carving.operation = SSOL_AND;
  carving.nb_vertices = npolygon_verts;
  carving.context = &polygon;
  quadric.type = SSOL_QUADRIC_PLANE;
  punched.nb_carvings = 1;
  punched.quadric = &quadric;
  punched.carvings = &carving;
  CHECK(ssol_shape_create_punched_surface(sol_dev, &shape2), RES_OK);
  CHECK(ssol_punched_surface_setup(shape2, &punched), RES_OK);
  CHECK(ssol_object_add_shaded_shape(obj2, shape2, mtl, mtl), RES_OK);
  CHECK(score_node_object_setup(geom2, obj2), RES_OK);

  CHECK(ssol_material_ref_put(mtl), RES_OK);
  CHECK(ssol_object_ref_put(obj1), RES_OK);
  CHECK(ssol_object_ref_put(obj2), RES_OK);
  CHECK(ssol_shape_ref_put(shape1), RES_OK);
  CHECK(ssol_shape_ref_put(shape2), RES_OK);
  CHECK(ssol_device_ref_put(sol_dev), RES_OK);

  CHECK(score_node_add_child(NULL, geom1), RES_BAD_ARG);
  CHECK(score_node_add_child(temp, NULL), RES_BAD_ARG);
  CHECK(score_node_add_child(tgt, geom1), RES_BAD_ARG);
  CHECK(score_node_add_child(inst, geom1), RES_BAD_ARG);
  CHECK(score_node_add_child(geom1, temp), RES_BAD_ARG);
  CHECK(score_node_add_child(geom1, inst), RES_BAD_ARG);
  CHECK(score_node_add_child(temp, geom1), RES_OK);
  CHECK(score_node_add_child(geom1, geom1), RES_BAD_ARG);
  CHECK(score_node_add_child(geom1, piv), RES_OK);
  CHECK(score_node_add_child(piv, piv2), RES_BAD_ARG);
  score_node_ref_put(piv2);

  score_node_set_translation(inst, transl);
  score_node_set_translation(piv, transl);
  score_node_set_translation(geom1, transl);
  score_node_set_translation(geom2, transl);
  score_node_set_translation(tgt, transl);

  score_node_set_rotations(inst, rot);
  score_node_set_rotations(piv, rot);
  score_node_set_rotations(geom1, rot);
  score_node_set_rotations(geom2, rot);

  score_node_ref_put(inst);
  score_node_ref_put(piv);
  score_node_ref_put(geom1);
  score_node_ref_put(geom2);
  score_node_ref_put(temp);
  score_node_ref_put(tgt);

  score_device_ref_put(dev);

  logger_release(&logger);
  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
