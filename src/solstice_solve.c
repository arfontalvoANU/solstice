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
  struct ssol_estimator_status status;
  struct htable_receiver_iterator it, end;
  const struct solparser_sun* solparser_sun = NULL;
  double irradiance_factor;
  ASSERT(solstice && estimator);

  /* get global information */
  SSOL(estimator_get_status(estimator, SSOL_STATUS_SHADOW, &status));
  SSOL(estimator_get_primary_area(estimator, &irradiance_factor));
  solparser_sun = solparser_get_sun(solstice->parser);
  irradiance_factor = 1 / (solparser_sun->dni * irradiance_factor);

  fprintf(solstice->output, "%lu %lu %lu\n",
    (unsigned long)htable_receiver_size_get(&solstice->receivers),
    (unsigned long)status.N,
    (unsigned long)status.Nf);

  htable_receiver_begin(&solstice->receivers, &it);
  htable_receiver_end(&solstice->receivers, &end);
  while(!htable_receiver_iterator_eq(&it, &end)) {
    const struct str* name = htable_receiver_iterator_key_get(&it);
    struct solstice_receiver* rcv = htable_receiver_iterator_data_get(&it);
    struct ssol_instance* inst = rcv->node->instance;
    struct ssol_estimator_status front;
    struct ssol_estimator_status back;
    uint32_t id;
    double f_eff_E, f_eff_SE, b_eff_E, b_eff_SE;

    htable_receiver_iterator_next(&it);
    switch(rcv->side) {
      case SRCVL_FRONT:
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_FRONT, &front));
        back.irradiance.E = back.irradiance.SE = -1;
        back.reflectivity_loss.E = back.reflectivity_loss.SE = -1;
        back.absorptivity_loss.E = back.absorptivity_loss.SE = -1;
        f_eff_E = front.irradiance.E * irradiance_factor;
        f_eff_SE = front.irradiance.SE * irradiance_factor;
        b_eff_E = b_eff_SE = -1;
        break;
      case SRCVL_BACK:
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_BACK, &back));
        front.irradiance.E = front.irradiance.SE = -1;
        front.reflectivity_loss.E = front.reflectivity_loss.SE = -1;
        front.absorptivity_loss.E = front.absorptivity_loss.SE = -1;
        f_eff_E = f_eff_SE = -1;
        b_eff_E = back.irradiance.E * irradiance_factor;
        b_eff_SE = back.irradiance.SE * irradiance_factor;
        break;
      case SRCVL_FRONT_AND_BACK:
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_FRONT, &front));
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_BACK, &back));
        f_eff_E = front.irradiance.E * irradiance_factor;
        f_eff_SE = front.irradiance.SE * irradiance_factor;
        b_eff_E = back.irradiance.E * irradiance_factor;
        b_eff_SE = back.irradiance.SE * irradiance_factor;
        break;
      default: FATAL("Unreachable code.\n"); break;
    }
    SSOL(instance_get_id(inst, &id));
    fprintf(solstice->output, "%s %u   %g %g %g %g   %g %g %g %g   %g %g %g %g   %g %g %g %g\n",
      str_cget(name), (unsigned) id,
      front.irradiance.E, front.irradiance.SE,
      back.irradiance.E, back.irradiance.SE,
      front.reflectivity_loss.E, front.reflectivity_loss.SE,
      back.reflectivity_loss.E, back.reflectivity_loss.SE,
      front.absorptivity_loss.E, front.absorptivity_loss.SE,
      back.absorptivity_loss.E, back.absorptivity_loss.SE,
      /* global efficiency */
      f_eff_E, f_eff_SE, b_eff_E, b_eff_SE);
  }

  SSOL(estimator_get_status(estimator, SSOL_STATUS_SHADOW, &status));
  fprintf(solstice->output, "%g %g\n", status.irradiance.E, status.irradiance.SE);
  SSOL(estimator_get_status(estimator, SSOL_STATUS_MISSING, &status));
  fprintf(solstice->output, "%g %g\n", status.irradiance.E, status.irradiance.SE);
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

