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

  res = ssol_sun_set_pillbox_aperture
    (sun, MDEG2RAD(solparser_sun->radang_distrib.pillbox.aperture));
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the aperture of the solver pillbox sun.\n");
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
  const struct solparser_spectrum_data* specdata = ctx;
  ASSERT(wlen && data && ctx);
  *wlen = specdata[i].wavelength;
  *data = specdata[i].data;
}

static res_T
create_sun_spectrum
  (struct solstice* solstice,
   const struct solparser_sun* solparser_sun,
   struct ssol_spectrum** out_spectrum)
{
  struct ssol_spectrum* spectrum = NULL;
  const struct solparser_spectrum_data* data;
  size_t nwlens;
  res_T res = RES_OK;
  ASSERT(solstice && solparser_sun && out_spectrum);

  res = ssol_spectrum_create(solstice->ssol, &spectrum);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the spectrum of the solver sun.\n");
    goto error;
  }

  nwlens = darray_spectrum_data_size_get(&solparser_sun->spectrum);
  data = darray_spectrum_data_cdata_get(&solparser_sun->spectrum);
  res = ssol_spectrum_setup(spectrum, get_wavelength, nwlens, (void*)data);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the spectrum of the solver sun.\n");
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
    default: FATAL("Unreachable code.\n"); break;
  }
  if(res != RES_OK) goto error;

  if(solparser_sun->spectrum.size) {
    res = create_sun_spectrum(solstice, solparser_sun, &spectrum);
    if (res != RES_OK) goto error;

    res = ssol_sun_set_spectrum(sun, spectrum);
    if(res != RES_OK) {
      fprintf(stderr, "Could not attach the spectrum to the sun.\n");
      goto error;
    }
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

