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

#include <star/ssp.h>

#include <rsys/logger.h>
#include <rsys/double3.h>

int
main(int argc, char** argv)
{
  struct logger logger;
  struct mem_allocator allocator;
  struct score_device* dev = NULL;
  struct score_scene* scene = NULL;
  struct score_node *temp1 = NULL, *inst1 = NULL, *temp2 = NULL, *inst2 = NULL,
    *piv = NULL, *geom1 = NULL, *geom2 = NULL, *tgt = NULL;
  struct sanim_pivot pivot = SANIM_PIVOT_NULL;
  struct sanim_tracking tracking = SANIM_TRACKING_NULL;
  struct ssol_device* sol_dev = NULL;
  struct ssol_punched_surface punched = SSOL_PUNCHED_SURFACE_NULL;
  struct ssol_carving carving = SSOL_CARVING_NULL;
  struct ssol_quadric quadric = SSOL_QUADRIC_DEFAULT;
  struct ssol_shape *shape1 = NULL, *shape2 = NULL;
  struct ssol_material *v_mtl = NULL, *m_mtl = NULL;
  struct ssol_object *obj1 = NULL, *obj2 = NULL;
  struct ssol_sun* sun = NULL;
  struct ssol_estimator* estimator = NULL;
  struct ssol_spectrum* spectrum = NULL;
  struct ssol_vertex_data attribs[1] = { SSOL_VERTEX_DATA_NULL__ };
  struct ssol_mirror_shader shader = SSOL_MIRROR_SHADER_NULL;
  struct ssp_rng* rng = NULL;
  double sun_dir[3];
  double transl[3], rot[3];
  double polygon [] = {
    -1.0, -1.0, -1.0, 1.0, 1.0, 1.0, 1.0, -1.0, 0.f, -2.f
  };
  double wavelengths[3] = { 1, 2, 3 };
  double intensities[3] = { 1, 0.8, 1 };
  const size_t npolygon_verts = sizeof(polygon) / sizeof(double[2]);
  FILE* tmp = NULL;
  (void) argc, (void) argv;

  mem_init_proxy_allocator(&allocator, &mem_default_allocator);

  CHECK(logger_init(&allocator, &logger), RES_OK);
  logger_set_stream(&logger, LOG_OUTPUT, log_stream, NULL);
  logger_set_stream(&logger, LOG_ERROR, log_stream, NULL);
  logger_set_stream(&logger, LOG_WARNING, log_stream, NULL);

  CHECK(ssol_device_create(
    &logger, &allocator, SSOL_NTHREADS_DEFAULT, 0, &sol_dev), RES_OK);

  CHECK(score_device_create(&logger, &allocator, 1, 0, &dev), RES_OK);

  /* create a template of a virtual 'target' */

  attribs[0].usage = SSOL_POSITION;
  attribs[0].get = get_position;
  CHECK(ssol_shape_create_mesh(sol_dev, &shape2), RES_OK);
  CHECK(ssol_mesh_setup(shape2, RECT_NTRIS__, get_ids, RECT_NVERTS__,
    attribs, 1, (void*) &RECT_DESC__), RES_OK);
  CHECK(ssol_material_create_virtual(sol_dev, &v_mtl), RES_OK);
  CHECK(ssol_object_create(sol_dev, &obj2), RES_OK);
  CHECK(ssol_object_add_shaded_shape(obj2, shape2, v_mtl, v_mtl), RES_OK);
  CHECK(score_node_create_object(dev, &geom2), RES_OK);
  CHECK(score_node_object_setup(geom2, obj2), RES_OK);

  CHECK(score_node_template_create(dev, &temp2), RES_OK);
  CHECK(score_node_add_child(temp2, geom2), RES_OK);
  /* define a tracking target at the geom2 center */
  CHECK(score_node_tracking_target_create(dev, &tgt), RES_OK);
  CHECK(score_node_add_child(geom2, tgt), RES_OK);
  score_node_set_receiver(geom2, SSOL_FRONT);
  score_node_sample(geom2, 0);
  d3(transl, 10, 0, 3);
  score_node_set_translation(geom2, transl);
  d3(rot, 0, PI / 2, 0);
  score_node_set_rotations(geom2, rot);
  CHECK(score_node_instantiate(temp2, &inst2), RES_OK);

  CHECK(ssol_material_ref_put(v_mtl), RES_OK);
  CHECK(ssol_object_ref_put(obj2), RES_OK);
  CHECK(ssol_shape_ref_put(shape2), RES_OK);

  /* create a template of a 'heliostat' */

  carving.get = get_polygon_vertices;
  carving.operation = SSOL_AND;
  carving.nb_vertices = npolygon_verts;
  carving.context = &polygon;
  quadric.type = SSOL_QUADRIC_PLANE;
  punched.nb_carvings = 1;
  punched.quadric = &quadric;
  punched.carvings = &carving;
  CHECK(ssol_shape_create_punched_surface(sol_dev, &shape1), RES_OK);
  CHECK(ssol_punched_surface_setup(shape1, &punched), RES_OK);
  CHECK(ssol_object_create(sol_dev, &obj1), RES_OK);
  CHECK(ssol_material_create_mirror(sol_dev, &m_mtl), RES_OK);
  shader.normal = get_shader_normal;
  shader.reflectivity = get_shader_reflectivity;
  shader.roughness = get_shader_roughness;
  CHECK(ssol_mirror_set_shader(m_mtl, &shader), RES_OK);
  CHECK(ssol_object_add_shaded_shape(obj1, shape1, m_mtl, m_mtl), RES_OK);
  CHECK(score_node_create_object(dev, &geom1), RES_OK);

  score_node_track_me(tgt, &tracking);
  pivot.type = PIVOT_SINGLE_AXIS;
  d3(pivot.data.pivot1.ref_point, 0, 0, 0);
  d3(pivot.data.pivot1.ref_normal, 0, 0, 1);
  CHECK(score_node_object_setup(geom1, obj1), RES_OK);

  CHECK(score_node_pivot_create(dev, &piv), RES_OK);
  CHECK(score_node_pivot_setup(piv, &pivot, &tracking), RES_OK);
  d3(transl, 0, 0, 3);
  score_node_set_translation(piv, transl);
  d3(rot, 0, 0, PI / 2);
  score_node_set_rotations(piv, rot);

  CHECK(score_node_template_create(dev, &temp1), RES_OK);
  CHECK(score_node_add_child(temp1, piv), RES_OK);
  CHECK(score_node_add_child(piv, geom1), RES_OK);
  CHECK(score_node_instantiate(temp1, &inst1), RES_OK);

  CHECK(ssol_material_ref_put(m_mtl), RES_OK);
  CHECK(ssol_object_ref_put(obj1), RES_OK);
  CHECK(ssol_shape_ref_put(shape1), RES_OK);
  CHECK(ssol_device_ref_put(sol_dev), RES_OK);

  /* some scene API tests */
  CHECK(score_scene_create(dev, &scene), RES_OK);
  score_scene_ref_put(scene);

  CHECK(score_scene_create(dev, &scene), RES_OK);

#define N__ 10000
  NCHECK(tmp = tmpfile(), 0);
  CHECK(ssp_rng_create(&allocator, &ssp_rng_threefry, &rng), RES_OK);
  CHECK(ssol_estimator_create(sol_dev, &estimator), RES_OK);
  CHECK(score_scene_attach_instance(scene, inst1), RES_OK);

  score_scene_detach_instance(scene, inst1);

  /* create and attach a sun, including API tests */

  CHECK(ssol_spectrum_create(sol_dev, &spectrum), RES_OK);
  CHECK(ssol_spectrum_setup(spectrum, wavelengths, intensities, 3), RES_OK);
  CHECK(ssol_sun_create_directional(sol_dev, &sun), RES_OK);
  d3(sun_dir, 0, 0, -1);
  CHECK(ssol_sun_set_direction(sun, sun_dir), RES_OK);
  CHECK(ssol_sun_set_spectrum(sun, spectrum), RES_OK);
  CHECK(ssol_sun_set_dni(sun, 1000), RES_OK);
  score_scene_attach_sun(scene, sun);
  score_scene_detach_sun(scene, sun);

  CHECK(ssol_spectrum_ref_put(spectrum), RES_OK);

  /* fill up the scene */

  CHECK(score_scene_attach_instance(scene, inst1), RES_OK);
  CHECK(score_scene_attach_instance(scene, inst2), RES_OK);
  /* no sun to get a direction to track */
  score_scene_attach_sun(scene, sun);
  CHECK(score_scene_reset_simulation(scene), RES_OK);

  CHECK(fclose(tmp), 0);

  /* clean up memory */

  CHECK(ssol_sun_ref_put(sun), RES_OK);
  CHECK(ssp_rng_ref_put(rng), RES_OK);
  CHECK(ssol_estimator_ref_put(estimator), RES_OK);

  score_scene_ref_put(scene);

  score_node_ref_put(inst1);
  score_node_ref_put(inst2);
  score_node_ref_put(piv);
  score_node_ref_put(geom1);
  score_node_ref_put(geom2);
  score_node_ref_put(temp1);
  score_node_ref_put(temp2);
  score_node_ref_put(tgt);
  score_device_ref_put(dev);

  logger_release(&logger);
  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
