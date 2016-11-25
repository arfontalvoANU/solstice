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
#include "solstice_core_device.h"

#include <rsys/logger.h>
#include <rsys/mem_allocator.h>

#include <solstice/ssol.h>

#include <omp.h>

/*******************************************************************************
* Helper functions
******************************************************************************/
static INLINE void
log_msg
  (struct score_device* dev,
   const enum log_type stream,
   const char* msg,
   va_list vargs)
{
  ASSERT(dev && msg);
  if (dev->verbose) {
    res_T res; (void) res;
    res = logger_vprint(dev->logger, stream, msg, vargs);
    ASSERT(res == RES_OK);
  }
}

static void
device_release(ref_T* ref)
{
  struct score_device* dev;
  ASSERT(ref);
  dev = CONTAINER_OF(ref, struct score_device, ref);
  ASSERT(dev && dev->allocator);
  if (dev->ssol) ssol_device_ref_put(dev->ssol);
  MEM_RM(dev->allocator, dev);
}

/*******************************************************************************
 * Exported score_device functions
 ******************************************************************************/
res_T
score_device_create
  (struct logger* logger,
   struct mem_allocator* mem_allocator,
   const unsigned nthreads_hint,
   const int verbose,
   struct score_device** out_dev)
{
  struct score_device* dev = NULL;
  struct mem_allocator* allocator;
  res_T res = RES_OK;

  ASSERT(nthreads_hint && out_dev);

  allocator = mem_allocator ? mem_allocator : &mem_default_allocator;
  dev = MEM_CALLOC(allocator, 1, sizeof(struct score_device));
  if (!dev) {
    res = RES_MEM_ERR;
    goto error;
  }
  ref_init(&dev->ref);
  dev->logger = logger ? logger : LOGGER_DEFAULT;
  dev->allocator = allocator;
  dev->verbose = verbose;
  dev->nthreads = MMIN(nthreads_hint, (unsigned) omp_get_num_procs());
  omp_set_num_threads((int) dev->nthreads);

  res = ssol_device_create(logger, allocator, nthreads_hint, verbose, &dev->ssol);
  if (res != RES_OK) goto error;

exit:
  if (out_dev) *out_dev = dev;
  return res;
error:
  if (dev) {
    score_device_ref_put(dev);
    dev = NULL;
  }
  goto exit;
}

void
score_device_ref_get(struct score_device* dev)
{
  ASSERT(dev);
  ref_get(&dev->ref);
}

void
score_device_ref_put(struct score_device* dev)
{
  ASSERT(dev);
  ref_put(&dev->ref, device_release);
}

struct ssol_device*
score_device_get_solver_device(struct score_device* dev)
{
  ASSERT(dev);
  return dev->ssol;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
void
log_error(struct score_device* dev, const char* msg, ...)
{
  va_list vargs_list;
  ASSERT(dev && msg);

  va_start(vargs_list, msg);
  log_msg(dev, LOG_ERROR, msg, vargs_list);
  va_end(vargs_list);
}

void
log_warning(struct score_device* dev, const char* msg, ...)
{
  va_list vargs_list;
  ASSERT(dev && msg);

  va_start(vargs_list, msg);
  log_msg(dev, LOG_WARNING, msg, vargs_list);
  va_end(vargs_list);
}

