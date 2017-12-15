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

#include <rsys/double33.h>
#include <rsys/image.h>
#include <solstice/ssol.h>

struct dielectric_param {
  struct ssol_image* normal_map;
};

struct matte_param {
  struct ssol_data reflectivity;
  struct ssol_image* normal_map;
};

struct mirror_param {
  struct ssol_data reflectivity;
  struct ssol_data roughness;
  struct ssol_image* normal_map;
};

struct thin_dielectric_param {
  struct ssol_image* normal_map;
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
perturb_normal
  (const struct ssol_surface_fragment* frag,
   const struct ssol_image* normal_map,
   double normal[3])
{
  double basis[9];
  double N[3];
  ASSERT(frag && normal_map && normal);

  SSOL(image_sample(normal_map, SSOL_FILTER_LINEAR, SSOL_ADDRESS_CLAMP,
    SSOL_ADDRESS_CLAMP, frag->uv, N));

  d3_set(basis+0, frag->dPdu);
  d3_set(basis+3, frag->dPdv);
  d3_set(basis+6, frag->Ns);
  d3_normalize(basis + 0, basis + 0);
  d3_normalize(basis + 3, basis + 3);

  d3_subd(N, d3_muld(N, N, 2), 1);
  d33_muld3(N, basis, N);
  d3_normalize(normal, N);
}

static enum ssol_microfacet_distribution
solparser_to_ssol_ufacet_distrib
  (const enum solparser_microfacet_distribution distrib)
{
  enum ssol_microfacet_distribution ufacet_distrib;
  switch(distrib) {
    case SOLPARSER_MICROFACET_BECKMANN:
      ufacet_distrib = SSOL_MICROFACET_BECKMANN;
      break;
    case SOLPARSER_MICROFACET_PILLBOX:
      ufacet_distrib = SSOL_MICROFACET_PILLBOX;
      break;
    default: FATAL("Unreachable code.\n"); break;
  }
  return ufacet_distrib;
}

static void
mtl_get_normal
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  (void)dev, (void)buf, (void)wavelength;
  d3_set(val, frag->Ns);
}

static void
dielectric_get_normal
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  const struct dielectric_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)buf, (void)wavelength;
  perturb_normal(frag, param->normal_map, val);
}

static void
dielectric_param_release(void* mem)
{
  struct dielectric_param* param = mem;
  ASSERT(param);
  if(param->normal_map) SSOL(image_ref_put(param->normal_map));
}

static void
matte_get_reflectivity
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  const struct matte_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)frag;
  *val = ssol_data_get_value(&param->reflectivity, wavelength);
}

static void
matte_get_normal
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  const struct matte_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength;
  perturb_normal(frag, param->normal_map, val);
}

static void
matte_param_release(void* mem)
{
  struct matte_param* param = mem;
  ASSERT(param);
  if(param->normal_map) SSOL(image_ref_put(param->normal_map));
  ssol_data_clear(&param->reflectivity);
}

static void
mirror_get_reflectivity
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  const struct mirror_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)frag;
  *val = ssol_data_get_value(&param->reflectivity, wavelength);
}

static void
mirror_get_roughness
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  const struct mirror_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)frag;
  *val = ssol_data_get_value(&param->roughness, wavelength);
}

static void
mirror_get_normal
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  const struct mirror_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength;
  perturb_normal(frag, param->normal_map, val);
}

static void
mirror_param_release(void* mem)
{
  struct mirror_param* param = mem;
  ASSERT(param);
  if(param->normal_map) SSOL(image_ref_put(param->normal_map));
  ssol_data_clear(&param->reflectivity);
  ssol_data_clear(&param->roughness);
}

static void
thin_dielectric_get_normal
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const struct ssol_surface_fragment* frag,
   double* val)
{
  const struct thin_dielectric_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)buf, (void)wavelength;
  perturb_normal(frag, param->normal_map, val);
}

static void
thin_dielectric_param_release(void* mem)
{
  struct thin_dielectric_param* param = mem;
  ASSERT(param);
  if(param->normal_map) SSOL(image_ref_put(param->normal_map));
}

static res_T
load_image
  (struct solstice* solstice,
   const char* filename,
   struct ssol_image** out_ssol_img)
{
  struct ssol_image* ssol_img = NULL;
  struct ssol_image_layout layout;
  struct image img;
  char* mem;
  size_t x, y;
  res_T res = RES_OK;
  ASSERT(solstice && filename && out_ssol_img);

  image_init(solstice->allocator, &img);

  res = image_read_ppm(&img, filename);
  if(res != RES_OK) {
    fprintf(stderr, "Could not load the PPM image `%s'.\n", filename);
    goto error;
  }

  res = ssol_image_create(solstice->ssol, &ssol_img);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver image.\n");
    goto error;
  }

  res = ssol_image_setup(ssol_img, img.width, img.height, SSOL_PIXEL_DOUBLE3);
  if(res != RES_OK) {
    fprintf(stderr, "Could not setup the Solstice Solver image.\n");
    goto error;
  }

  SSOL(image_get_layout(ssol_img, &layout));
  SSOL(image_map(ssol_img, &mem));

  FOR_EACH(y, 0, layout.height) {
    char* dst_row = mem + layout.offset + y * layout.row_pitch;
    const char* src_row = img.pixels + y * img.pitch;

    FOR_EACH(x, 0, layout.width) {
      char* dst = dst_row + x*ssol_sizeof_pixel_format(layout.pixel_format);
      const char* src = src_row + x*sizeof_image_format(img.format);
      switch(img.format) {
        case IMAGE_RGB8:
          ((double*)dst)[0] = ((double)((uint8_t*)src)[0] + 0.5) / UINT8_MAX;
          ((double*)dst)[1] = ((double)((uint8_t*)src)[1] + 0.5) / UINT8_MAX;
          ((double*)dst)[2] = ((double)((uint8_t*)src)[2] + 0.5) / UINT8_MAX;
          break;
        case IMAGE_RGB16:
          ((double*)dst)[0] = ((double)((uint16_t*)src)[0] + 0.5) / UINT16_MAX;
          ((double*)dst)[1] = ((double)((uint16_t*)src)[1] + 0.5) / UINT16_MAX;
          ((double*)dst)[2] = ((double)((uint16_t*)src)[2] + 0.5) / UINT16_MAX;
          break;
        default: FATAL("Unreachable code.\n"); break;
      }
    }
  }

exit:
  image_release(&img);
  *out_ssol_img = ssol_img;
  return res;
error:
  if(ssol_img) SSOL(image_ref_put(ssol_img));
  goto exit;
}

static res_T
create_material_dielectric
  (struct solstice* solstice,
   const struct solparser_material_dielectric* dielectric,
   struct ssol_material** out_mtl)
{
  const struct solparser_medium* medium_i;
  const struct solparser_medium* medium_t;
  struct ssol_medium ssol_medium_i = SSOL_MEDIUM_VACUUM;
  struct ssol_medium ssol_medium_t = SSOL_MEDIUM_VACUUM;
  struct ssol_dielectric_shader shader = SSOL_DIELECTRIC_SHADER_NULL;
  struct ssol_image* img = NULL;
  struct ssol_material* mtl = NULL;
  struct ssol_param_buffer* pbuf = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && dielectric && out_mtl);

  res = ssol_material_create_dielectric(solstice->ssol, &mtl);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not allocate the Solstice Solver dielectric material.\n");
    goto error;
  }

  medium_i = solparser_get_medium(solstice->parser, dielectric->medium_i);
  medium_t = solparser_get_medium(solstice->parser, dielectric->medium_t);

  if(!SOLPARSER_ID_IS_VALID(dielectric->normal_map)) {
    shader.normal = mtl_get_normal;
  } else {
    const struct solparser_image* image;
    struct dielectric_param* param = NULL;

    image = solparser_get_image(solstice->parser, dielectric->normal_map);
    res = load_image(solstice, str_cget(&image->filename), &img);
    if(res != RES_OK) goto error;

    res = ssol_param_buffer_create
      (solstice->ssol, sizeof(struct dielectric_param), &pbuf);
    if(res != RES_OK) {
      fprintf(stderr, "Could not create the Solstice Solver parameter buffer.\n");
      goto error;
    }

    param = ssol_param_buffer_allocate(pbuf, sizeof(struct dielectric_param),
      ALIGNOF(struct dielectric_param), dielectric_param_release);
    if(!param) {
      fprintf(stderr, "Could not allocate the dielectric parameter.\n");
      res = RES_MEM_ERR;
      goto error;
    }
    param->normal_map = img;
    shader.normal = dielectric_get_normal;
    SSOL(material_set_param_buffer(mtl, pbuf));
  }

  #define SET_SSOL_DATA(Medium, Name) {                                        \
    res = mtl_to_ssol_data(solstice, &Medium->Name, &ssol_## Medium.Name);     \
    if(res != RES_OK) goto error;                                              \
  } (void)0
  SET_SSOL_DATA(medium_i, refractive_index);
  SET_SSOL_DATA(medium_t, refractive_index);
  SET_SSOL_DATA(medium_i, extinction);
  SET_SSOL_DATA(medium_t, extinction);
  #undef SET_SSOL_DATA
  SSOL(dielectric_setup(mtl, &shader, &ssol_medium_i, &ssol_medium_t));

exit:
  ssol_medium_clear(&ssol_medium_i);
  ssol_medium_clear(&ssol_medium_t);
  if(pbuf) SSOL(param_buffer_ref_put(pbuf));
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  if(img) SSOL(image_ref_put(img));
  goto exit;
}

static res_T
create_material_matte
  (struct solstice* solstice,
   const struct solparser_material_matte* matte,
   struct ssol_material** out_mtl)
{
  struct ssol_matte_shader shader = SSOL_MATTE_SHADER_NULL;
  struct ssol_image* img = NULL;
  struct ssol_material* mtl = NULL;
  struct ssol_param_buffer* pbuf = NULL;
  struct matte_param* param;
  res_T res = RES_OK;
  ASSERT(solstice && matte && out_mtl);

  res = ssol_material_create_matte(solstice->ssol, &mtl);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver matte material.\n");
    goto error;
  }

  res = ssol_param_buffer_create
    (solstice->ssol, sizeof(struct matte_param), &pbuf);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver parameter buffer.\n");
    goto error;
  }

  param = ssol_param_buffer_allocate(pbuf, sizeof(struct matte_param),
    ALIGNOF(struct matte_param), matte_param_release);
  if(!param) {
    fprintf(stderr, "Could not allocate the matte parameter.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  memset(param, 0, sizeof(struct matte_param));

  res = mtl_to_ssol_data(solstice, &matte->reflectivity, &param->reflectivity);
  if(res != RES_OK) goto error;

  if(!SOLPARSER_ID_IS_VALID(matte->normal_map)) {
    shader.normal = mtl_get_normal;
  } else {
    const struct solparser_image* image;
    image = solparser_get_image(solstice->parser, matte->normal_map);
    res = load_image(solstice, str_cget(&image->filename), &img);
    if(res != RES_OK) goto error;
    param->normal_map = img;
    shader.normal = matte_get_normal;
  }

  shader.reflectivity = matte_get_reflectivity;
  SSOL(matte_setup(mtl, &shader));
  SSOL(material_set_param_buffer(mtl, pbuf));

exit:
  if(pbuf) SSOL(param_buffer_ref_put(pbuf));
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  if(img) SSOL(image_ref_put(img));
  goto exit;
}

static res_T
create_material_mirror
  (struct solstice* solstice,
   const struct solparser_material_mirror* mirror,
   struct ssol_material** out_mtl)
{
  struct ssol_image* img = NULL;
  struct ssol_mirror_shader shader = SSOL_MIRROR_SHADER_NULL;
  struct ssol_material* mtl = NULL;
  struct ssol_param_buffer* pbuf = NULL;
  struct mirror_param* param;
  enum ssol_microfacet_distribution ufacet_distrib;
  res_T res = RES_OK;
  ASSERT(solstice && mirror && out_mtl);

  res = ssol_material_create_mirror(solstice->ssol, &mtl);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver mirror material.\n");
    goto error;
  }

  res = ssol_param_buffer_create
    (solstice->ssol, sizeof(struct mirror_param), &pbuf);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver parameter buffer.\n");
    goto error;
  }

  param = ssol_param_buffer_allocate(pbuf, sizeof(struct mirror_param),
    ALIGNOF(struct mirror_param), mirror_param_release);
  if(!param) {
    fprintf(stderr, "Could not allocate the mirror parameters.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  memset(param, 0, sizeof(struct mirror_param));

  res = mtl_to_ssol_data(solstice, &mirror->reflectivity, &param->reflectivity);
  if(res != RES_OK) goto error;

  switch(mirror->ufacet_distrib) {
    case SOLPARSER_MICROFACET_BECKMANN:
      /* For the beckmann distribution, convert the slope error to the
       * corresponding beckmann rougness by multiplying it by sqrt(2) */
      res = scaled_mtl_to_ssol_data
        (solstice, &mirror->slope_error, sqrt(2), &param->roughness);
      break;
    case SOLPARSER_MICROFACET_PILLBOX:
      /* Direct correspondance between the solver roughness parameter and the
       * provided slope error */
      res = mtl_to_ssol_data(solstice, &mirror->slope_error, &param->roughness);
      break;
    default: FATAL("Unreachable code.\n"); break;
  }

  if(!SOLPARSER_ID_IS_VALID(mirror->normal_map)) {
    shader.normal = mtl_get_normal;
  } else {
    const struct solparser_image* image;
    image = solparser_get_image(solstice->parser, mirror->normal_map);
    res = load_image(solstice, str_cget(&image->filename), &img);
    if(res != RES_OK) goto error;
    param->normal_map = img;
    shader.normal = mirror_get_normal;
  }

  shader.reflectivity = mirror_get_reflectivity;
  shader.roughness = mirror_get_roughness;

  ufacet_distrib = solparser_to_ssol_ufacet_distrib(mirror->ufacet_distrib);
  SSOL(mirror_setup(mtl, &shader, ufacet_distrib));
  SSOL(material_set_param_buffer(mtl, pbuf));

exit:
  if(pbuf) SSOL(param_buffer_ref_put(pbuf));
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  if(img) SSOL(image_ref_put(img));
  goto exit;
}

static res_T
create_material_thin_dielectric
  (struct solstice* solstice,
   const struct solparser_material_thin_dielectric* thin,
   struct ssol_material** out_mtl)
{
  struct ssol_image* img = NULL;
  struct ssol_thin_dielectric_shader shader = SSOL_THIN_DIELECTRIC_SHADER_NULL;
  const struct solparser_medium* medium_i = NULL;
  const struct solparser_medium* medium_t = NULL;
  struct ssol_medium ssol_medium_i = SSOL_MEDIUM_VACUUM;
  struct ssol_medium ssol_medium_t = SSOL_MEDIUM_VACUUM;
  struct ssol_material* mtl = NULL;
  struct ssol_param_buffer* pbuf = NULL;
  res_T res = RES_OK;
  ASSERT(solstice && thin && out_mtl);

  res = ssol_material_create_thin_dielectric(solstice->ssol, &mtl);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not allocate the Solstice Solver thin dielectric material.\n");
    goto error;
  }

  medium_i = solparser_get_medium(solstice->parser, thin->medium_i);
  medium_t = solparser_get_medium(solstice->parser, thin->medium_t);

  if(!SOLPARSER_ID_IS_VALID(thin->normal_map)) {
    shader.normal = mtl_get_normal;
  } else {
    const struct solparser_image* image;
    struct dielectric_param* param = NULL;

    image = solparser_get_image(solstice->parser, thin->normal_map);
    res = load_image(solstice, str_cget(&image->filename), &img);
    if(res != RES_OK) goto error;

    res = ssol_param_buffer_create
      (solstice->ssol, sizeof(struct thin_dielectric_param), &pbuf);
    if(res != RES_OK) {
      fprintf(stderr, "Could not create the Solsitce Solver parameter buffer.\n");
      goto error;
    }

    param = ssol_param_buffer_allocate(pbuf,
      sizeof(struct thin_dielectric_param),
      ALIGNOF(struct thin_dielectric_param),
      thin_dielectric_param_release);
    if(!param) {
      fprintf(stderr, "Could not allocate the thin dielectric parameter.\n");
      res = RES_MEM_ERR;
      goto error;
    }
    param->normal_map = img;
    shader.normal = thin_dielectric_get_normal;
    SSOL(material_set_param_buffer(mtl, pbuf));
  }

  #define SET_SSOL_DATA(Medium, Name) {                                        \
    res = mtl_to_ssol_data(solstice, &Medium->Name, &ssol_## Medium.Name);     \
    if(res != RES_OK) goto error;                                              \
  } (void)0
  SET_SSOL_DATA(medium_i, refractive_index);
  SET_SSOL_DATA(medium_t, refractive_index);
  SET_SSOL_DATA(medium_i, extinction);
  SET_SSOL_DATA(medium_t, extinction);
  #undef SET_SSOL_DATA
  SSOL(thin_dielectric_setup
    (mtl, &shader, &ssol_medium_i, &ssol_medium_t, thin->thickness));

exit:
  ssol_medium_clear(&ssol_medium_i);
  ssol_medium_clear(&ssol_medium_t);
  if(pbuf) SSOL(param_buffer_ref_put(pbuf));
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  if(img) SSOL(image_ref_put(img));
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
scaled_mtl_to_ssol_data
  (struct solstice* solstice,
   const struct solparser_mtl_data* mtl_data,
   const double scale_factor,
   struct ssol_data* data)
{
  struct ssol_spectrum* spectrum = NULL;
  res_T res = RES_OK;
  ASSERT(mtl_data && data);

  ssol_data_clear(data);
  switch(mtl_data->type) {
    case SOLPARSER_MTL_DATA_REAL:
      ssol_data_set_real(data, mtl_data->value.real*scale_factor);
      break;
    case SOLPARSER_MTL_DATA_SPECTRUM:
      res = solstice_create_scaled_ssol_spectrum
        (solstice, mtl_data->value.spectrum, scale_factor, &spectrum);
      if(res != RES_OK) goto error;
      ssol_data_set_spectrum(data, spectrum);
      break;
    default: FATAL("Unreachable code.\n"); break;
  }

exit:
  if(spectrum) SSOL(spectrum_ref_put(spectrum));
  return res;
error:
  ssol_data_clear(data);
  goto exit;
}

extern LOCAL_SYM res_T
mtl_to_ssol_data
  (struct solstice* solstice,
   const struct solparser_mtl_data* mtl_data,
   struct ssol_data* data)
{
  return scaled_mtl_to_ssol_data(solstice, mtl_data, 1.0, data);
}

res_T
solstice_create_ssol_material
  (struct solstice* solstice,
   const struct solparser_material_id mtl_id,
   struct ssol_material** out_ssol_mtl)
{
  const struct solparser_material* mtl;
  struct ssol_material* ssol_mtl = NULL;
  struct ssol_material** pssol_mtl = NULL;
  res_T res = RES_OK;
  ASSERT(solstice);

  mtl = solparser_get_material(solstice->parser, mtl_id);
  ASSERT(mtl);

  if(mtl->type == SOLPARSER_MATERIAL_VIRTUAL) {
    /* Use the global solstice virtual material */
    ssol_mtl = solstice->mtl_virtual;
  } else {
    pssol_mtl = htable_material_find(&solstice->materials, &mtl_id.i);
    if(pssol_mtl) {
      ssol_mtl = *pssol_mtl;
    } else {
      const struct solparser_material_dielectric* dielectric;
      const struct solparser_material_matte* matte;
      const struct solparser_material_mirror* mirror;
      const struct solparser_material_thin_dielectric* thin_dielectric;

      switch(mtl->type) {
        case SOLPARSER_MATERIAL_DIELECTRIC:
          dielectric = solparser_get_material_dielectric
            (solstice->parser, mtl->data.dielectric);
          res = create_material_dielectric(solstice, dielectric, &ssol_mtl);
          break;
        case SOLPARSER_MATERIAL_MATTE:
          matte = solparser_get_material_matte
            (solstice->parser, mtl->data.matte);
          res = create_material_matte(solstice, matte, &ssol_mtl);
          break;
        case SOLPARSER_MATERIAL_MIRROR:
          mirror = solparser_get_material_mirror
            (solstice->parser, mtl->data.mirror);
          res = create_material_mirror(solstice, mirror, &ssol_mtl);
          break;
        case SOLPARSER_MATERIAL_THIN_DIELECTRIC:
          thin_dielectric = solparser_get_material_thin_dielectric
            (solstice->parser, mtl->data.thin_dielectric);
          res = create_material_thin_dielectric
            (solstice, thin_dielectric, &ssol_mtl);
          break;
        default: FATAL("Unreachable code.\n"); break;
      }
      if(res != RES_OK) goto error;

      /* Cache the created material for future use. */
      res = htable_material_set(&solstice->materials, &mtl_id.i, &ssol_mtl);
      if(res != RES_OK) {
        fprintf(stderr, "Could not register the material into solstice.\n");
        goto error;
      }
    }
  }

  /* Get an additional reference onto the material in order to give to the
   * caller an ownership onto the returned material. */
  SSOL(material_ref_get(ssol_mtl));

exit:
  *out_ssol_mtl = ssol_mtl;
  return res;
error:
  if(ssol_mtl) SSOL(material_ref_put(ssol_mtl));
  goto exit;
}

