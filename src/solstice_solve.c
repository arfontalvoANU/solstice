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

  fprintf(solstice->output, "%lu\n",
    (unsigned long)htable_receiver_size_get(&solstice->receivers));

  while(!htable_receiver_iterator_eq(&it, &end)) {
    struct solstice_receiver* receiver = htable_receiver_iterator_data_get(&it);
    uint32_t id;
    SSOL(instance_get_id(receiver->node->instance, &id));
    fprintf(solstice->output, "%u %s\n", id, str_cget(&receiver->name));
    htable_receiver_iterator_next(&it);
  }
}

static void
write_global_mc(struct solstice* solstice, struct ssol_estimator* estimator)
{
  struct ssol_estimator_status status;
  struct htable_receiver_iterator it, end;
  ASSERT(solstice && estimator);

  htable_receiver_begin(&solstice->receivers, &it);
  htable_receiver_end(&solstice->receivers, &end);
  while(!htable_receiver_iterator_eq(&it, &end)) {
    struct solstice_receiver* rcv = htable_receiver_iterator_data_get(&it);
    struct ssol_instance* inst = rcv->node->instance;
    struct ssol_estimator_status front;
    struct ssol_estimator_status back;
    uint32_t id;

    htable_receiver_iterator_next(&it);
    switch(rcv->side) {
      case SRCVL_FRONT:
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_FRONT, &front));
        back.E = back.SE = -1;
        break;
      case SRCVL_BACK:
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_BACK, &back));
        front.E = front.SE = -1;
        break;
      case SRCVL_FRONT_AND_BACK:
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_FRONT, &front));
        SSOL(estimator_get_receiver_status(estimator, inst, SSOL_BACK, &back));
        break;
      default: FATAL("Unreachable code.\n"); break;
    }
    SSOL(instance_get_id(inst, &id));
    fprintf(solstice->output, "%u %g %g %g %g\n", (unsigned)id,
      front.E, front.SE, back.E, back.SE);
  }

  SSOL(estimator_get_status(estimator, SSOL_STATUS_SHADOW, &status));
  fprintf(solstice->output, "%g %g\n", status.E, status.SE);
  SSOL(estimator_get_status(estimator, SSOL_STATUS_MISSING, &status));
  fprintf(solstice->output, "%g %g\n", status.E, status.SE);
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

  bin_stream = tmpfile();
  if(!bin_stream) {
    fprintf(stderr, "Could not create the temporary output binary stream.\n");
    res = RES_IO_ERR;
    goto error;
  }

  res = ssol_estimator_create(solstice->ssol, &estimator);
  if(res != RES_OK) {
    fprintf(stderr, "Error in creating the Solstice Estimator.\n");
    goto error;
  }

  write_header(solstice);

  res = ssol_solve(solstice->scene, rng, solstice->nrealisations,
    /*bin_stream FIXME*/NULL, estimator);
  if(res != RES_OK) {
    fprintf(stderr, "Error in integrating the solar flux.\n");
    goto error;
  }

  write_global_mc(solstice, estimator);

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

exit:
  if(bin_stream) fclose(bin_stream);
  if(estimator) SSOL(estimator_ref_put(estimator));
  if(rng) SSP(rng_ref_put(rng));
  return res;
error:
  goto exit;
}

