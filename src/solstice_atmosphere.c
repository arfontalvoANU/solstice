/* Copyright (C) 2018, 2019, 2021 |Meso|Star> (contact@meso-star.com)
 * Copyright (C) 2016-2018 CNRS
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
#include "parser/solparser_atmosphere.h"

#include <solstice/ssol.h>

res_T
solstice_create_atmosphere(struct solstice* solstice)
{
  struct ssol_atmosphere* atm = NULL;
  struct ssol_data extinction = SSOL_DATA_NULL;
  const struct solparser_atmosphere* solparser_atm = NULL;
  res_T res = RES_OK;
  ASSERT(solstice);

  solparser_atm = solparser_get_atmosphere(solstice->parser);
  if(!solparser_atm) return res;

  res = ssol_atmosphere_create(solstice->ssol, &atm);
  if(res != RES_OK) goto error;
  
  res = mtl_to_ssol_data(solstice, &solparser_atm->extinction, &extinction);
  if(res != RES_OK) goto error;

  res = ssol_atmosphere_set_extinction(atm, &extinction);
  if(res != RES_OK) {
    fprintf(stderr, "Could not set atmosphere extinction.\n");
    goto error;
  }

  res = ssol_scene_attach_atmosphere(solstice->scene, atm);
  if(res != RES_OK) {
    fprintf(stderr, "Could not attach the atmosphere to the scene.\n");
    goto error;
  }

exit:
  ssol_data_clear(&extinction);
  solstice->atmosphere = atm;
  return res;
error:
  if(atm) {
    SSOL(atmosphere_ref_put(atm));
    atm = NULL;
  }

  goto exit;
}
