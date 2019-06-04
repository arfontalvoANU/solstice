/* Copyright (C) 2016-2018 CNRS, 2018-2019 |Meso|Star>
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

#include <rsys/image.h>
#include <solstice/ssol.h>

#define SCREEN_GAMMA 2.2

/*******************************************************************************
 * Helper function
 ******************************************************************************/
/* Assume that the pixel format of the src is DOUBLE3 and dst is UBYTE3 */
static void
tone_map(const double* src, uint8_t* dst, const size_t count)
{
  size_t i;
  ASSERT(src && dst && count);
  FOR_EACH(i, 0, count) {
    double val[3];
    val[0] = pow(src[i*3/*#channels*/+0], 1/SCREEN_GAMMA);/* Gamma correction */
    val[1] = pow(src[i*3/*#channels*/+1], 1/SCREEN_GAMMA);/* Gamma correction */
    val[2] = pow(src[i*3/*#channels*/+2], 1/SCREEN_GAMMA);/* Gamma correction */
    val[0] = CLAMP(val[0], 0, 1);
    val[1] = CLAMP(val[1], 0, 1);
    val[2] = CLAMP(val[2], 0, 1);
    dst[i*3/*#channels*/ + 0] = (uint8_t)((val[0]*255) + 0.5/*round*/);
    dst[i*3/*#channels*/ + 1] = (uint8_t)((val[1]*255) + 0.5/*round*/);
    dst[i*3/*#channels*/ + 2] = (uint8_t)((val[2]*255) + 0.5/*round*/);
  }
}

static void
tone_map_image(const struct ssol_image* img, uint8_t* dst)
{
  struct ssol_image_layout layout;
  size_t irow = 0;
  char* mem;
  ASSERT(img && dst);

  SSOL(image_get_layout(img, &layout));
  ASSERT(layout.pixel_format == SSOL_PIXEL_DOUBLE3);

  SSOL(image_map(img, &mem));
  FOR_EACH(irow, 0, layout.height) {
    const void* src_row = ((char*)mem) + layout.offset + irow * layout.row_pitch;
    uint8_t* dst_row = dst + irow * layout.width * 3/*#channels*/;
    tone_map(src_row, dst_row, layout.width);
  }
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solstice_draw(struct solstice* solstice)
{
  struct ssol_image_layout layout;
  struct image img;
  size_t pitch;
  res_T res = RES_OK;
  ASSERT(solstice);

  SSOL(image_get_layout(solstice->framebuffer, &layout));

  pitch = layout.width * sizeof_image_format(IMAGE_RGB8);
  image_init(solstice->allocator, &img);
  res = image_setup(&img, layout.width, layout.height, pitch, IMAGE_RGB8, NULL);
  if(res != RES_OK) {
    fprintf(stderr, "Could not allocate the 8-bits image buffer.\n");
    res = RES_MEM_ERR;
    goto error;
  }

  switch(solstice->render_mode) {
    case SOLSTICE_ARGS_RENDER_DRAFT:
      res = ssol_draw_draft(solstice->scene, solstice->camera, layout.width,
        layout.height, solstice->spp, ssol_image_write, solstice->framebuffer);
      break;
    case SOLSTICE_ARGS_RENDER_PATH_TRACING:
      res = ssol_draw_pt(solstice->scene, solstice->camera, layout.width,
        layout.height, solstice->spp, solstice->up, ssol_image_write,
        solstice->framebuffer);
      break;
    default: FATAL("Unreachable code.\n");
  }
  if(res != RES_OK) {
    fprintf(stderr, "Rendering error\n");
    goto error;
  }

  tone_map_image(solstice->framebuffer, (uint8_t*)img.pixels);

  res = image_write_ppm_stream(&img, 0, solstice->output);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not write the rendered image to the output stream.\n");
    goto error;
  }

exit:
  image_release(&img);
  return res;
error:
  goto exit;
}

