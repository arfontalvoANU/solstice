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
write_global_mc(struct solstice* solstice, struct ssol_estimator* estimator)
{
  #define MC_RECEIVER_NULL {                                                   \
    { -1, -1, -1 }, /* integrated_irradiance */                                \
    { -1, -1, -1 }, /* absorptivity_loss */                                    \
    { -1, -1, -1 }, /* reflectivity_loss */                                    \
    { -1, -1, -1 }, /* coss_loss */                                            \
    0, NULL                                                                    \
  }
  struct ssol_mc_global mc_global;
  struct htable_receiver_iterator it, end;
  const struct solparser_sun* solparser_sun = NULL;
  size_t nexperiments;
  double irradiance_factor;
  ASSERT(solstice && estimator);

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
    struct ssol_mc_receiver front = MC_RECEIVER_NULL;
    struct ssol_mc_receiver back = MC_RECEIVER_NULL;
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
      str_cget(name), (unsigned) id,
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

  res = ssol_solve(solstice->scene, rng, solstice->nrealisations,
    bin_stream, &estimator);
  if(res != RES_OK) {
    fprintf(stderr, "Error in integrating the solar flux.\n");
    goto error;
  }

  write_global_mc(solstice, estimator);

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

