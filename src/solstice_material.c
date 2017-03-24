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

struct matte_param {
  double reflectivity;
};

struct mirror_param {
  double reflectivity;
  double roughness;
};

struct thin_dielectric_param {
  double absorption;
  double thickness;
  double refractive_index;
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static void
mtl_get_normal
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  (void)dev, (void)buf, (void)wavelength, (void)P, (void)Ng, (void)uv, (void)w;
  val[0] = Ns[0];
  val[1] = Ns[1];
  val[2] = Ns[2];
}

static void
matte_get_reflectivity
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  const struct matte_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)P, (void)Ng, (void)Ns, (void)uv, (void)w;
  *val = param->reflectivity;
}

static void
mirror_get_reflectivity
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  const struct mirror_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)P, (void)Ng, (void)Ns, (void)uv, (void)w;
  *val = param->reflectivity;
}

static void
mirror_get_roughness
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  const struct mirror_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)P, (void)Ng, (void)Ns, (void)uv, (void)w;
  *val = param->roughness;
}

static void
thin_dielectric_get_absorption
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  const struct thin_dielectric_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)P, (void)Ng, (void)Ns, (void)uv, (void)w;
  *val = param->absorption;
}

static void
thin_dielectric_get_thickness
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  const struct thin_dielectric_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)P, (void)Ng, (void)Ns, (void)uv, (void)w;
  *val = param->thickness;
}

static void
thin_dielectric_get_refractive_index
  (struct ssol_device* dev,
   struct ssol_param_buffer* buf,
   const double wavelength,
   const double P[3],
   const double Ng[3],
   const double Ns[3],
   const double uv[2],
   const double w[3],
   double* val)
{
  const struct thin_dielectric_param* param = ssol_param_buffer_get(buf);
  (void)dev, (void)wavelength, (void)P, (void)Ng, (void)Ns, (void)uv, (void)w;
  *val = param->refractive_index;
}

static res_T
create_material_dielectric
  (struct solstice* solstice,
   const struct solparser_material_dielectric* dielectric,
   struct ssol_material** out_mtl)
{
  const struct solparser_medium* medium_i;
  const struct solparser_medium* medium_t;
  struct ssol_dielectric_shader shader = SSOL_DIELECTRIC_SHADER_NULL;
  struct ssol_material* mtl = NULL;
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
  shader.normal = mtl_get_normal;
  SSOL(dielectric_setup(mtl, &shader, 
    medium_i->refractive_index,
    medium_t->refractive_index,
    medium_i->absorptivity,
    medium_t->absorptivity));

exit:
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  goto exit;
}

static res_T
create_material_matte
  (struct solstice* solstice,
   const struct solparser_material_matte* matte,
   struct ssol_material** out_mtl)
{
  struct ssol_matte_shader shader = SSOL_MATTE_SHADER_NULL;
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

  param = ssol_param_buffer_allocate
    (pbuf, sizeof(struct matte_param), ALIGNOF(struct matte_param));
  if(!param) {
    fprintf(stderr, "Could not allocate the matte parameter.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  param->reflectivity = matte->reflectivity;

  shader.normal = mtl_get_normal;
  shader.reflectivity = matte_get_reflectivity;
  SSOL(matte_set_shader(mtl, &shader));
  SSOL(material_set_param_buffer(mtl, pbuf));

exit:
  if(pbuf) SSOL(param_buffer_ref_put(pbuf));
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  goto exit;
}

static res_T
create_material_mirror
  (struct solstice* solstice,
   const struct solparser_material_mirror* mirror,
   struct ssol_material** out_mtl)
{
  struct ssol_mirror_shader shader = SSOL_MIRROR_SHADER_NULL;
  struct ssol_material* mtl = NULL;
  struct ssol_param_buffer* pbuf = NULL;
  struct mirror_param* param;
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

  param = ssol_param_buffer_allocate
    (pbuf, sizeof(struct mirror_param), ALIGNOF(struct mirror_param));
  if(!param) {
    fprintf(stderr, "Could not allocate the mirror parameters.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  param->reflectivity = mirror->reflectivity;
  param->roughness = mirror->roughness;

  shader.normal = mtl_get_normal;
  shader.reflectivity = mirror_get_reflectivity;
  shader.roughness = mirror_get_roughness;
  SSOL(mirror_set_shader(mtl, &shader));
  SSOL(material_set_param_buffer(mtl, pbuf));

exit:
  if(pbuf) SSOL(param_buffer_ref_put(pbuf));
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  goto exit;
}

static res_T
create_material_thin_dielectric
  (struct solstice* solstice,
   const struct solparser_material_thin_dielectric* thin,
   struct ssol_material** out_mtl)
{
  struct ssol_thin_dielectric_shader shader = SSOL_THIN_DIELECTRIC_SHADER_NULL;
  struct ssol_material* mtl = NULL;
  struct ssol_param_buffer* pbuf = NULL;
  struct thin_dielectric_param* param;
  res_T res = RES_OK;
  ASSERT(solstice && thin && out_mtl);

  res = ssol_material_create_thin_dielectric(solstice->ssol, &mtl);
  if(res != RES_OK) {
    fprintf(stderr,
      "Could not allocate the Solstice Solver thin dielectric material.\n");
    goto error;
  }

  res = ssol_param_buffer_create
    (solstice->ssol, sizeof(struct thin_dielectric_param), &pbuf);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver parameter buffer.\n");
    goto error;
  }

  param = ssol_param_buffer_allocate(pbuf,
    sizeof(struct thin_dielectric_param), ALIGNOF(struct thin_dielectric_param));
  if(!param) {
    fprintf(stderr, "Could not allocate the thin dielectric parameters.\n");
    res = RES_MEM_ERR;
    goto error;
  }

  param->thickness = thin->thickness;
  param->absorption = thin->absorption;
  param->refractive_index = thin->refractive_index;

  shader.normal = mtl_get_normal;
  shader.absorption = thin_dielectric_get_absorption;
  shader.thickness = thin_dielectric_get_thickness;
  shader.refractive_index = thin_dielectric_get_refractive_index;
  SSOL(thin_dielectric_set_shader(mtl, &shader));
  SSOL(material_set_param_buffer(mtl, pbuf));

exit:
  if(pbuf) SSOL(param_buffer_ref_put(pbuf));
  *out_mtl = mtl;
  return res;
error:
  if(mtl) SSOL(material_ref_put(mtl)), mtl = NULL;
  goto exit;
}


/*******************************************************************************
 * Local functions
 ******************************************************************************/
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

