/* Copyright (C) 2016-2018 CNRS
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

struct context {
  double scale_factor;
  const struct solparser_spectrum_data* spectrum_data;
};

/*******************************************************************************
 * Helper function
 ******************************************************************************/
static void
get_wavelength(const size_t i, double* wlen, double* data, void* context)
{
  const struct context* ctx = context;
  ASSERT(wlen && data && context);
  *wlen = ctx->spectrum_data[i].wavelength;
  *data = ctx->spectrum_data[i].data * ctx->scale_factor;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_create_ssol_spectrum
  (struct solstice* solstice,
   const struct solparser_spectrum_id spectrum_id,
   struct ssol_spectrum** out_spectrum)
{
  return solstice_create_scaled_ssol_spectrum
    (solstice, spectrum_id, 1, out_spectrum);
}

res_T
solstice_create_scaled_ssol_spectrum
  (struct solstice* solstice,
   const struct solparser_spectrum_id spectrum_id,
   const double scale_factor,
   struct ssol_spectrum** out_spectrum)
{
  struct context ctx;
  struct ssol_spectrum* spectrum = NULL;
  const struct solparser_spectrum* spec;
  size_t nwlens;
  res_T res = RES_OK;
  ASSERT(solstice && SOLPARSER_ID_IS_VALID(spectrum_id) && out_spectrum);

  res = ssol_spectrum_create(solstice->ssol, &spectrum);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver spectrum.\n");
    goto error;
  }

  spec = solparser_get_spectrum(solstice->parser, spectrum_id);
  nwlens = darray_spectrum_data_size_get(&spec->data);
  ctx.spectrum_data = darray_spectrum_data_cdata_get(&spec->data);
  ctx.scale_factor = scale_factor;
  res = ssol_spectrum_setup(spectrum, get_wavelength, nwlens, &ctx);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the Solstice Solver spectrum.\n");
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

