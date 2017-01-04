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

#include <solstice/ssol.h>

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_draw(struct solstice* solstice)
{
  struct ssol_image_layout layout;
  res_T res = RES_OK;
  ASSERT(solstice);

  SSOL(image_get_layout(solstice->framebuffer, &layout));

  res = ssol_draw(solstice->scene, solstice->camera, layout.width,
    layout.height, ssol_image_write, solstice->framebuffer);
  if(res != RES_OK) {
    fprintf(stderr, "Rendering error\n");
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}
