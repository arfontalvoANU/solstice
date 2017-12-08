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
#include "solstice_sun_spectrum.h"
#include "parser/solparser.h"
#include "parser/solparser_sun.h"

#include <solstice/ssol.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static res_T
create_sun_buie
  (struct solstice* solstice,
   const struct solparser_sun* solparser_sun,
   struct ssol_sun** out_sun)
{
  struct ssol_sun* sun = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && solparser_sun && out_sun);
  ASSERT(solparser_sun->radang_distrib_type == SOLPARSER_SUN_RADANG_DISTRIB_BUIE);

  res = ssol_sun_create_buie(solstice->ssol, &sun);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the solver buie sun.\n");
    goto error;
  }

  res = ssol_sun_set_buie_param(sun, solparser_sun->radang_distrib.buie.csr);
  if(res != RES_OK) {
    fprintf(stderr, "Could setup the buie parameter of the solver sun.\n");
    goto error;
  }

exit:
  *out_sun = sun;
  return res;
error:
  if(sun) {
    SSOL(sun_ref_put(sun));
    sun = NULL;
  }
  goto exit;
}

static res_T
create_sun_dir
  (struct solstice* solstice,
   const struct solparser_sun* solparser_sun,
   struct ssol_sun** out_sun)
{
  struct ssol_sun* sun = NULL;
  res_T res = RES_OK;
  (void)solparser_sun;
  ASSERT(solstice && solparser_sun && out_sun);
  ASSERT(solparser_sun->radang_distrib_type
      == SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL);

  res = ssol_sun_create_directional(solstice->ssol, &sun);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the solver directional sun.\n");
    goto error;
  }

exit:
  *out_sun = sun;
  return res;
error:
  if(sun) {
    SSOL(sun_ref_put(sun));
    sun = NULL;
  }
  goto exit;
}

static res_T
create_sun_pillbox
  (struct solstice* solstice,
   const struct solparser_sun* solparser_sun,
   struct ssol_sun** out_sun)
{
  struct ssol_sun* sun = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && solparser_sun && out_sun);
  ASSERT(solparser_sun->radang_distrib_type
      == SOLPARSER_SUN_RADANG_DISTRIB_PILLBOX);

  res = ssol_sun_create_pillbox(solstice->ssol, &sun);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the solver pillbox sun.\n");
    goto error;
  }

  res = ssol_sun_pillbox_set_half_angle
    (sun, MDEG2RAD(solparser_sun->radang_distrib.pillbox.half_angle));
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup half_angle for the solver pillbox sun.\n");
    goto error;
  }

exit:
  *out_sun = sun;
  return res;
error:
  if(sun) {
    SSOL(sun_ref_put(sun));
    sun = NULL;
  }
  goto exit;
}

static res_T
create_sun_gaussian
  (struct solstice* solstice,
   const struct solparser_sun* solparser_sun,
   struct ssol_sun** out_sun)
{
  struct ssol_sun* sun = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && solparser_sun && out_sun);
  ASSERT(solparser_sun->radang_distrib_type
    == SOLPARSER_SUN_RADANG_DISTRIB_GAUSSIAN);

  res = ssol_sun_create_gaussian(solstice->ssol, &sun);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the solver gaussian sun.\n");
    goto error;
  }

  res = ssol_sun_gaussian_set_std_dev
    (sun, MDEG2RAD(solparser_sun->radang_distrib.gaussian.std_dev));
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup standard deviation for the solver gaussian sun.\n");
    goto error;
  }

exit:
  *out_sun = sun;
  return res;
error:
  if(sun) {
    SSOL(sun_ref_put(sun));
    sun = NULL;
  }
  goto exit;
}

static void
get_wavelength(const size_t i, double* wlen, double* data, void* ctx)
{
  const double* spectrum = ctx;
  ASSERT(wlen && data && ctx);
  *wlen = spectrum[i*2+0];
  *data = spectrum[i*2+1];
}

static res_T
create_default_sun_spectrum
  (struct solstice* solstice,
   const struct solparser_sun* solparser_sun,
   struct ssol_spectrum** out_spectrum)
{
  struct ssol_spectrum* spectrum = NULL;
  const double* data;
  size_t size;
  res_T res = RES_OK;

  /* The solparser_sun may be used if the default spectrum is defined wrt the
   * sun type */
  (void)solparser_sun;

  res = ssol_spectrum_create(solstice->ssol, &spectrum);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the spectrum of the solver sun.\n");
    goto error;
  }

  if(solparser_has_spectrum(solstice->parser)) {
    data = solstice_sun_spectrum_smarts;
    size = solstice_sun_spectrum_smarts_size;
  } else {
    data = solstice_sun_spectrum_dummy;
    size = solstice_sun_spectrum_dummy_size;
  }

  res = ssol_spectrum_setup(spectrum, get_wavelength, size, (void*)data);
  if(res != RES_OK) {
    fprintf(stderr, "Coul not setup the default spectrum of the solver sun.\n");
    goto error;
  }

exit:
  *out_spectrum = spectrum;
  return res;
error:
  if(spectrum) {
    SSOL(spectrum_ref_put(spectrum));
    spectrum = NULL;
  }
  goto exit;
}

/*******************************************************************************
 * Local function
 ******************************************************************************/
res_T
solstice_create_sun(struct solstice* solstice)
{
  struct ssol_sun* sun = NULL;
  struct ssol_spectrum* spectrum = NULL;
  const struct solparser_sun* solparser_sun = NULL;
  res_T res = RES_OK;
  ASSERT(solstice);

  solparser_sun = solparser_get_sun(solstice->parser);
  switch(solparser_sun->radang_distrib_type) {
    case SOLPARSER_SUN_RADANG_DISTRIB_BUIE:
      res = create_sun_buie(solstice, solparser_sun, &sun);
      break;
    case SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL:
      res = create_sun_dir(solstice, solparser_sun, &sun);
      break;
    case SOLPARSER_SUN_RADANG_DISTRIB_PILLBOX:
      res = create_sun_pillbox(solstice, solparser_sun, &sun);
      break;
    case SOLPARSER_SUN_RADANG_DISTRIB_GAUSSIAN:
      res = create_sun_gaussian(solstice, solparser_sun, &sun);
      break;
    default: FATAL("Unreachable code.\n"); break;
  }
  if(res != RES_OK) goto error;

  if(!SOLPARSER_ID_IS_VALID(solparser_sun->spectrum)) {
    res = create_default_sun_spectrum(solstice, solparser_sun, &spectrum);
    if(res != RES_OK) goto error;
  } else {
    res = solstice_create_ssol_spectrum
      (solstice, solparser_sun->spectrum, &spectrum);
    if(res != RES_OK) goto error;
  }

  res = ssol_sun_set_spectrum(sun, spectrum);
  if(res != RES_OK) {
    fprintf(stderr, "Could not attach the spectrum to the sun.\n");
    goto error;
  }

  res = ssol_sun_set_dni(sun, solparser_sun->dni);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the DNI of the sun.\n");
    goto error;
  }

  res = ssol_scene_attach_sun(solstice->scene, sun);
  if(res != RES_OK) {
    fprintf(stderr, "Could not attach the sun to the scene.\n");
    goto error;
  }

exit:
  if(spectrum) SSOL(spectrum_ref_put(spectrum));
  solstice->sun = sun;
  return res;
error:
  if(sun) {
    SSOL(sun_ref_put(sun));
    sun = NULL;
  }

  goto exit;
}

