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

#ifndef SOLSTICE_CORE_H
#define SOLSTICE_CORE_H

#include <rsys/rsys.h>
#include <rsys/dynamic_array.h>

/* Forward declaration of external types */
struct logger;
struct mem_allocator;
struct sanim_pivot;
struct sanim_tracking;
struct ssol_vertex_data;
struct ssol_object;
struct ssol_instance;
struct ssol_sun;
struct ssol_atmosphere;
struct ssol_estimator;
struct ssol_device;
struct ssp_rng;

struct score_device;
struct score_node;

/*******************************************************************************
 * Device API - Main entry point of Solstice core. Applications
 * use the score_device to create others Solstice core resources.
 ******************************************************************************/
extern LOCAL_SYM res_T
score_device_create
  (struct logger* logger, /* May be NULL <=> use default logger */
   struct mem_allocator* allocator, /* May be NULL <=> use default allocator */
   const unsigned nthreads_hint, /* Hint on the number of threads to use */
   const int verbose, /* Make the library more verbose */
   struct score_device** dev);

extern LOCAL_SYM void
score_device_ref_get
  (struct score_device* dev);

extern LOCAL_SYM void
score_device_ref_put
  (struct score_device* dev);

extern LOCAL_SYM struct ssol_device*
score_device_get_solver_device
  (struct score_device* dev);

/*******************************************************************************
 * Node API
 ******************************************************************************/
extern LOCAL_SYM res_T
score_node_geometry_create
  (struct score_device* dev,
   struct score_node** geom);

extern LOCAL_SYM res_T
score_node_pivot_create
  (struct score_device* dev,
   struct score_node** node);

extern LOCAL_SYM res_T
score_node_empty_create
  (struct score_device* dev,
   struct score_node** node);

extern LOCAL_SYM res_T
score_node_tracking_target_create
  (struct score_device* dev,
   struct score_node** node);

extern LOCAL_SYM void
score_node_ref_get
  (struct score_node* node);

extern LOCAL_SYM void
score_node_ref_put
  (struct score_node* node);

extern LOCAL_SYM res_T
score_node_geometry_setup
  (struct score_node* node,
   struct ssol_instance* geom);

extern LOCAL_SYM res_T
score_node_pivot_setup
  (struct score_node* node,
   const struct sanim_pivot* pivot,
   const struct sanim_tracking* tracking);

extern LOCAL_SYM void
score_node_track_me
  (const struct score_node* node,
   struct sanim_tracking* tracking);

extern LOCAL_SYM res_T
score_node_add_child
  (struct score_node* father,
   struct score_node* child);

extern LOCAL_SYM void
score_node_set_translation
  (struct score_node* node,
   const double translation[3]);

extern LOCAL_SYM void
score_node_get_translation
  (const struct score_node* node,
    double translation[3]);

extern LOCAL_SYM void
score_node_set_rotations
  (struct score_node* node,
   const double rotations[3]);

extern LOCAL_SYM void
score_node_get_rotations
  (const struct score_node* node,
   double rotations[3]);

extern LOCAL_SYM void
score_node_set_receiver
  (struct score_node* node,
   const int mask);

/* Define whether or not the node is sampled.
 * By default a node is sampled. */
extern LOCAL_SYM void
score_node_sample
  (struct score_node* node,
   const int sample);

/*******************************************************************************
* Miscellaneous functions
******************************************************************************/
extern LOCAL_SYM void
score_scene_clear
  (struct score_device* dev);

extern LOCAL_SYM res_T
score_reset_simulation
  (struct score_device* dev,
   const double sun_dir[3]);

extern LOCAL_SYM res_T
score_update_simulation
  (struct score_device* dev,
   const double sun_dir[3]);

extern LOCAL_SYM res_T
score_solve
  (struct score_device* dev,
   struct ssp_rng* rng,
   const size_t realisations_count,
   FILE* output,
   struct ssol_estimator* estimator);

#endif /* SOLSTICE_CORE_H */

