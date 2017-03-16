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

#include "solstice.h"
#include "solstice_c.h"
#include "parser/solparser_sun.h"
#include <solstice/ssol.h>
#include <star/ssp.h>

/*******************************************************************************
 * Helper function
 ******************************************************************************/
static void
write_mc_global(struct solstice* solstice, struct ssol_estimator* estimator)
{
  struct ssol_mc_global mc_global;
  struct htable_receiver_iterator it, end;
  const struct solparser_sun* solparser_sun = NULL;
  size_t nexperiments;
  double irradiance_factor;
  ASSERT(solstice && estimator);

  #define MC_RCV_NONE {                                                        \
    { -1, -1, -1 }, /* Integrated irradiance  */                               \
    { -1, -1, -1 }, /* Absorptivity loss */                                    \
    { -1, -1, -1 }, /* Reflectivity loss */                                    \
    { -1, -1, -1 }, /* Cos loss */                                             \
    0, NULL, NULL                                                              \
  }

  /* get global information */
  SSOL(estimator_get_mc_global(estimator, &mc_global));
  SSOL(estimator_get_count(estimator, &nexperiments));
  SSOL(estimator_get_sampled_area(estimator, &irradiance_factor));
  solparser_sun = solparser_get_sun(solstice->parser);
  irradiance_factor = 1.0 / (solparser_sun->dni * irradiance_factor);

  fprintf(solstice->output, "%lu %lu\n",
    (unsigned long)htable_receiver_size_get(&solstice->receivers),
    (unsigned long)nexperiments);

  htable_receiver_begin(&solstice->receivers, &it);
  htable_receiver_end(&solstice->receivers, &end);
  while(!htable_receiver_iterator_eq(&it, &end)) {
    const struct str* name = htable_receiver_iterator_key_get(&it);
    struct solstice_receiver* rcv = htable_receiver_iterator_data_get(&it);
    struct ssol_instance* inst = rcv->node->instance;
    struct ssol_mc_receiver front = MC_RCV_NONE;
    struct ssol_mc_receiver back = MC_RCV_NONE;
    double f_eff_E = -1, f_eff_SE = -1; /* Front efficiency */
    double b_eff_E = -1, b_eff_SE = -1; /* Back efficiency */
    uint32_t id;

    htable_receiver_iterator_next(&it);
    switch(rcv->side) {
      case SRCVL_FRONT:
        SSOL(estimator_get_mc_receiver(estimator, inst, SSOL_FRONT, &front));
        f_eff_E = front.integrated_irradiance.E * irradiance_factor;
        f_eff_SE = front.integrated_irradiance.SE * irradiance_factor;
        break;
      case SRCVL_BACK:
        SSOL(estimator_get_mc_receiver(estimator, inst, SSOL_BACK, &back));
        b_eff_E = back.integrated_irradiance.E * irradiance_factor;
        b_eff_SE = back.integrated_irradiance.SE * irradiance_factor;
        break;
      case SRCVL_FRONT_AND_BACK:
        SSOL(estimator_get_mc_receiver(estimator, inst, SSOL_FRONT, &front));
        SSOL(estimator_get_mc_receiver(estimator, inst, SSOL_BACK, &back));
        f_eff_E = front.integrated_irradiance.E * irradiance_factor;
        f_eff_SE = front.integrated_irradiance.SE * irradiance_factor;
        b_eff_E = back.integrated_irradiance.E * irradiance_factor;
        b_eff_SE = back.integrated_irradiance.SE * irradiance_factor;
        break;
      default: FATAL("Unreachable code.\n"); break;
    }
    SSOL(instance_get_id(inst, &id));
    fprintf(solstice->output,
      "%s %u   %g %g %g %g   %g %g %g %g   %g %g %g %g   %g %g %g %g   %g %g %g %g\n",
      str_cget(name), (unsigned)id,
      front.integrated_irradiance.E, front.integrated_irradiance.SE,
      back.integrated_irradiance.E, back.integrated_irradiance.SE,
      front.reflectivity_loss.E, front.reflectivity_loss.SE,
      back.reflectivity_loss.E, back.reflectivity_loss.SE,
      front.absorptivity_loss.E, front.absorptivity_loss.SE,
      back.absorptivity_loss.E, back.absorptivity_loss.SE,
      front.cos_loss.E, front.cos_loss.SE,
      back.cos_loss.E, back.cos_loss.SE,
      f_eff_E, f_eff_SE, b_eff_E, b_eff_SE);
  }

  fprintf(solstice->output, "%g %g\n",
    mc_global.shadowed.E, mc_global.shadowed.SE);
  fprintf(solstice->output, "%g %g\n",
    mc_global.missing.E, mc_global.missing.SE);
}

static void
dump_instantiated_shaded_shape_vertices
  (struct solstice* solstice,
   const struct ssol_instantiated_shaded_shape* inst_sshape)
{
  unsigned ivert, nverts;
  ASSERT(solstice && inst_sshape);

  SSOL(shape_get_vertices_count(inst_sshape->shape, &nverts));
  FOR_EACH(ivert, 0, nverts) {
    double pos[3];
    SSOL(instantiated_shaded_shape_get_vertex_attrib
      (inst_sshape, ivert, SSOL_POSITION, pos));
    fprintf(solstice->output, "%f %f %f\n", SPLIT3(pos));
  }
}

static void
dump_shape_triangle_indices
  (struct solstice* solstice,
   const struct ssol_shape* shape,
   const size_t offset)
{
  unsigned itri, ntris;
  ASSERT(solstice && shape);

  SSOL(shape_get_triangles_count(shape, &ntris));
  FOR_EACH(itri, 0, ntris) {
    unsigned ids[3];
    SSOL(shape_get_triangle_indices(shape, itri, ids));
    fprintf(solstice->output, "3 %lu %lu %lu\n",
      (unsigned long)(ids[0] + offset),
      (unsigned long)(ids[1] + offset),
      (unsigned long)(ids[2] + offset));
  }
}

static void
dump_mc_shape
  (struct solstice* solstice,
   struct ssol_shape* shape,
   struct ssol_mc_shape* mc_shape)
{
  unsigned itri, ntris;
  ASSERT(solstice && shape && mc_shape);

  SSOL(shape_get_triangles_count(shape, &ntris));
  FOR_EACH(itri, 0, ntris) {
    struct ssol_mc_primitive mc_prim;
    SSOL(mc_shape_get_mc_primitive(mc_shape, itri, &mc_prim));
    fprintf(solstice->output, "%g %g\n",
      mc_prim.integrated_irradiance.E,
      mc_prim.integrated_irradiance.SE);
  }
}

static void
dump_per_primitive_mc_estimations
  (struct solstice* solstice,
   struct ssol_estimator* estimator,
   struct ssol_instance* inst,
   const enum ssol_side_flag side)
{
  size_t ishape, nshapes;
  struct ssol_mc_receiver mc_rcv;
  const char* name;
  ASSERT(solstice && estimator && inst);

  SSOL(estimator_get_mc_receiver(estimator, inst, side, &mc_rcv));

  switch(side) {
    case SSOL_FRONT: name = "Front_faces"; break;
    case SSOL_BACK: name = "Back_faces"; break;
    default: FATAL("Unreachable code.\n"); break;
  }

  fprintf(solstice->output, "SCALARS %s float 2\n", name);
  fprintf(solstice->output, "LOOKUP_TABLE default\n");

  SSOL(instance_get_shaded_shapes_count(inst, &nshapes));
  FOR_EACH(ishape, 0, nshapes) {
    struct ssol_instantiated_shaded_shape inst_sshape;
    struct ssol_mc_shape mc_shape;
    SSOL(instance_get_shaded_shape(inst, ishape, &inst_sshape));
    SSOL(mc_receiver_get_mc_shape(&mc_rcv, inst_sshape.shape, &mc_shape));
    dump_mc_shape(solstice, inst_sshape.shape, &mc_shape);
  }
}

static void
write_per_receiver_mc_primitive
  (struct solstice* solstice, struct ssol_estimator* estimator)
{
  struct htable_receiver_iterator it, end;
  ASSERT(solstice && estimator);

  htable_receiver_begin(&solstice->receivers, &it);
  htable_receiver_end(&solstice->receivers, &end);
  while(!htable_receiver_iterator_eq(&it, &end)) {
    struct ssol_instantiated_shaded_shape inst_sshape;
    const struct str* name = htable_receiver_iterator_key_get(&it);
    struct solstice_receiver* rcv = htable_receiver_iterator_data_get(&it);
    struct ssol_instance* inst = rcv->node->instance;
    size_t ishape, nshapes;
    size_t nverts, ntris;
    size_t offset;

    htable_receiver_iterator_next(&it);
    SSOL(instance_get_shaded_shapes_count(inst, &nshapes));

    /* Write the header */
    fprintf(solstice->output, "# vtk DataFile Version 2.0\n");
    fprintf(solstice->output, "%s\n", str_cget(name));
    fprintf(solstice->output, "ASCII\n");
    fprintf(solstice->output, "DATASET POLYDATA\n");

    /* Compute the overall number of vertices & triangles of the receiver */
    nverts = ntris = 0;
    FOR_EACH(ishape, 0, nshapes) {
      unsigned shape_nverts, shape_ntris;
      SSOL(instance_get_shaded_shape(inst, ishape, &inst_sshape));
      SSOL(shape_get_vertices_count(inst_sshape.shape, &shape_nverts));
      SSOL(shape_get_triangles_count(inst_sshape.shape, &shape_ntris));
      nverts += shape_nverts;
      ntris += shape_ntris;
    }

    /* Write the positions of the receiver shaded shapes */
    fprintf(solstice->output, "POINTS %lu float\n", (unsigned long)nverts);
    FOR_EACH(ishape, 0, nshapes) {
      SSOL(instance_get_shaded_shape(inst, ishape, &inst_sshape));
      dump_instantiated_shaded_shape_vertices(solstice, &inst_sshape);
    }

    /* Write the triangles of the receiver shade shapes */
    offset = 0;
    fprintf(solstice->output, "POLYGONS %lu %lu\n",
      (unsigned long)ntris, (unsigned long)ntris*4);
    FOR_EACH(ishape, 0, nshapes) {
      unsigned shape_nverts;
      SSOL(instance_get_shaded_shape(inst, ishape, &inst_sshape));
      SSOL(shape_get_vertices_count(inst_sshape.shape, &shape_nverts));
      dump_shape_triangle_indices(solstice, inst_sshape.shape, offset);
      offset += shape_nverts;
    }

    /* Write front faces MC estimations */
    fprintf(solstice->output, "CELL_DATA %lu\n", (unsigned long)ntris);
    switch(rcv->side) {
      case SRCVL_FRONT:
        dump_per_primitive_mc_estimations(solstice, estimator, inst, SSOL_FRONT);
        break;
      case SRCVL_BACK:
        dump_per_primitive_mc_estimations(solstice, estimator, inst, SSOL_BACK);
        break;
      case SRCVL_FRONT_AND_BACK:
        dump_per_primitive_mc_estimations(solstice, estimator, inst, SSOL_FRONT);
        dump_per_primitive_mc_estimations(solstice, estimator, inst, SSOL_BACK);
        break;
      default: FATAL("Unreachable code.\n"); break;
    }
  }
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_solve(struct solstice* solstice)
{
  char buf[1024];
  struct ssol_estimator* estimator = NULL;
  struct ssp_rng* rng = NULL;
  FILE* bin_stream = NULL;
  size_t sz;
  res_T res = RES_OK;
  ASSERT(solstice);

  res = ssp_rng_create(solstice->allocator, &ssp_rng_threefry, &rng);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Random Number Generator .\n");
    goto error;
  }

  if(solstice->output_hits) {
    bin_stream = tmpfile();
    if(!bin_stream) {
      fprintf(stderr, "Could not create the temporary output binary stream.\n");
      res = RES_IO_ERR;
      goto error;
    }
  }

  res = ssol_solve(solstice->scene, rng, solstice->nrealisations, 0,
    bin_stream, &estimator);
  if(res != RES_OK) {
    fprintf(stderr, "Error in integrating the solar flux.\n");
    goto error;
  }

  write_mc_global(solstice, estimator);
  write_per_receiver_mc_primitive(solstice, estimator);

  if(solstice->output_hits) {
    sz = (size_t)ftell(bin_stream);
    rewind(bin_stream);

    while(sz) {
      const size_t read_sz = MMIN(sz, sizeof(buf));
      if(fread(buf, 1, read_sz, bin_stream) != read_sz) {
        fprintf(stderr, "Could not read the output binary stream.\n");
        res = RES_IO_ERR;
        goto error;
      }
      if(fwrite(buf, 1, read_sz, solstice->output) != read_sz) {
        fprintf(stderr, "Could not write the output binary stream.\n");
        res = RES_IO_ERR;
        goto error;
      }
      sz -= read_sz;
    }
  }

exit:
  if(bin_stream) fclose(bin_stream);
  if(estimator) SSOL(estimator_ref_put(estimator));
  if(rng) SSP(rng_ref_put(rng));
  return res;
error:
  goto exit;
}

