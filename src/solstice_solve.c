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
#include <solstice/ssol.h>
#include <star/ssp.h>

/*******************************************************************************
 * Helper function
 ******************************************************************************/
static void
write_header(struct solstice* solstice)
{
  struct htable_receiver_iterator it, end;
  ASSERT(solstice);

  htable_receiver_begin(&solstice->receivers, &it);
  htable_receiver_end(&solstice->receivers, &end);

  while(!htable_receiver_iterator_eq(&it, &end)) {
    struct solstice_receiver* receiver = htable_receiver_iterator_data_get(&it);
    uint32_t id;
    SSOL(instance_get_id(receiver->node->instance, &id));
    fprintf(solstice->output, "%u %s\n", id, str_cget(&receiver->name));
    htable_receiver_iterator_next(&it);
  }
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_solve(struct solstice* solstice)
{
  struct ssol_estimator* estimator = NULL;
  struct ssol_estimator_status status;
  struct ssp_rng* rng = NULL;
  res_T res = RES_OK;
  ASSERT(solstice);

  res = ssp_rng_create(solstice->allocator, &ssp_rng_threefry, &rng);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Random Number Generator .\n");
    goto error;
  }

  res = ssol_estimator_create(solstice->ssol, &estimator);
  if(res != RES_OK) {
    fprintf(stderr, "Error in creating the Solstice Estimator.\n");
    goto error;
  }

  write_header(solstice);

  res = ssol_solve(solstice->scene, rng, solstice->nrealisations,
    solstice->output, estimator);
  if(res != RES_OK) {
    fprintf(stderr, "Error in integrating the solar flux.\n");
    goto error;
  }

exit:
  if(estimator) SSOL(estimator_ref_put(estimator));
  if(rng) SSP(rng_ref_put(rng));
  return res;
error:
  goto exit;
}

