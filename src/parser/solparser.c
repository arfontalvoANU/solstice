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

#define _POSIX_C_SOURCE 200112L /* nextafter support */

#include "solparser.h"
#include "solparser_entity.h"
#include "solparser_material.h"
#include "solparser_pivot.h"
#include "solparser_shape.h"
#include "solparser_sun.h"

#include <rsys/cstr.h>
#include <rsys/double3.h>
#include <rsys/dynamic_array.h>
#include <rsys/hash_table.h>
#include <rsys/ref_count.h>
#include <rsys/str.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

struct target_alias {
  struct solparser_pivot_id pivot;
  const yaml_node_t* alias; /* Anchor */
};

/* Declare the target_alias array */
#define DARRAY_NAME tgtalias
#define DARRAY_DATA struct target_alias
#include <rsys/dynamic_array.h>

/* Declare the array of matte materials */
#define DARRAY_NAME matte
#define DARRAY_DATA struct solparser_material_matte
#include <rsys/dynamic_array.h>

/* Declare the array of mirror materials */
#define DARRAY_NAME mirror
#define DARRAY_DATA struct solparser_material_mirror
#include <rsys/dynamic_array.h>

/* Declare the array of thin_dielectric materials */
#define DARRAY_NAME thin_dielectric
#define DARRAY_DATA struct solparser_material_thin_dielectric
#include <rsys/dynamic_array.h>

/* Declare the array of materials  */
#define DARRAY_NAME material
#define DARRAY_DATA struct solparser_material
#include <rsys/dynamic_array.h>

/* Declare the array of the double sided materials */
#define DARRAY_NAME material2
#define DARRAY_DATA struct solparser_material_double_sided
#include <rsys/dynamic_array.h>

/* Declare the array of the shapes */
#define DARRAY_NAME shape
#define DARRAY_DATA struct solparser_shape
#include <rsys/dynamic_array.h>

/* Declare the array of cuboid */
#define DARRAY_NAME cuboid
#define DARRAY_DATA struct solparser_shape_cuboid
#include <rsys/dynamic_array.h>

/* Declare the array of cylinder */
#define DARRAY_NAME cylinder
#define DARRAY_DATA struct solparser_shape_cylinder
#include <rsys/dynamic_array.h>

/* Declare the array of imported geometries */
#define DARRAY_NAME impgeom
#define DARRAY_DATA struct solparser_shape_imported_geometry
#define DARRAY_FUNCTOR_INIT solparser_shape_imported_geometry_init
#define DARRAY_FUNCTOR_RELEASE solparser_shape_imported_geometry_release
#define DARRAY_FUNCTOR_COPY solparser_shape_imported_geometry_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE \
  solparser_shape_imported_geometry_copy_and_release
#include <rsys/dynamic_array.h>

/* Declare the array of paraboloids */
#define DARRAY_NAME paraboloid
#define DARRAY_DATA struct solparser_shape_paraboloid
#define DARRAY_FUNCTOR_INIT solparser_shape_paraboloid_init
#define DARRAY_FUNCTOR_RELEASE solparser_shape_paraboloid_release
#define DARRAY_FUNCTOR_COPY solparser_shape_paraboloid_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE \
  solparser_shape_paraboloid_copy_and_release
#include <rsys/dynamic_array.h>

/* Declare the array of hyperboloids */
#define DARRAY_NAME hyperboloid
#define DARRAY_DATA struct solparser_shape_hyperboloid
#define DARRAY_FUNCTOR_INIT solparser_shape_hyperboloid_init
#define DARRAY_FUNCTOR_RELEASE solparser_shape_hyperboloid_release
#define DARRAY_FUNCTOR_COPY solparser_shape_hyperboloid_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE \
  solparser_shape_hyperboloid_copy_and_release
#include <rsys/dynamic_array.h>

/* Declare the array of planes */
#define DARRAY_NAME plane
#define DARRAY_DATA struct solparser_shape_plane
#define DARRAY_FUNCTOR_INIT solparser_shape_plane_init
#define DARRAY_FUNCTOR_RELEASE solparser_shape_plane_release
#define DARRAY_FUNCTOR_COPY solparser_shape_plane_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solparser_shape_plane_copy_and_release
#include <rsys/dynamic_array.h>

/* Declare the array of spheres */
#define DARRAY_NAME sphere
#define DARRAY_DATA struct solparser_shape_sphere
#include <rsys/dynamic_array.h>

/* Declare the array of objects */
#define DARRAY_NAME object
#define DARRAY_DATA struct solparser_object
#include <rsys/dynamic_array.h>

/* Declare the array of geometries */
#define DARRAY_NAME geometry
#define DARRAY_DATA struct solparser_geometry
#define DARRAY_FUNCTOR_INIT solparser_geometry_init
#define DARRAY_FUNCTOR_RELEASE solparser_geometry_release
#define DARRAY_FUNCTOR_COPY solparser_geometry_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solparser_geometry_copy_and_release
#include <rsys/dynamic_array.h>

/* Declare the array of entities */
#define DARRAY_NAME entity
#define DARRAY_DATA struct solparser_entity
#define DARRAY_FUNCTOR_INIT solparser_entity_init
#define DARRAY_FUNCTOR_RELEASE solparser_entity_release
#define DARRAY_FUNCTOR_COPY solparser_entity_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solparser_entity_copy_and_release
#include <rsys/dynamic_array.h>

/* Declare the array of anchors */
#define DARRAY_NAME anchor
#define DARRAY_DATA struct solparser_anchor
#define DARRAY_FUNCTOR_INIT solparser_anchor_init
#define DARRAY_FUNCTOR_RELEASE solparser_anchor_release
#define DARRAY_FUNCTOR_COPY solparser_anchor_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solparser_anchor_copy_and_release
#include <rsys/dynamic_array.h>

/* Declare the array of x_pivots */
#define DARRAY_NAME x_pivot
#define DARRAY_DATA struct solparser_x_pivot
#define DARRAY_FUNCTOR_INIT solparser_x_pivot_init
#include <rsys/dynamic_array.h>

/* Declare the array of zx_pivots */
#define DARRAY_NAME zx_pivot
#define DARRAY_DATA struct solparser_zx_pivot
#define DARRAY_FUNCTOR_INIT solparser_zx_pivot_init
#include <rsys/dynamic_array.h>

/* Declare the hash table that maps the address of a YAML node to the id of its
 * in memory representation. */
#define HTABLE_NAME yaml2sols
#define HTABLE_KEY yaml_node_t*
#define HTABLE_DATA size_t
#include <rsys/hash_table.h>

struct solparser {
  yaml_parser_t parser;
  struct str stream_name;
  int parser_is_init;

  /* Materia data */
  struct htable_yaml2sols yaml2mtls; /* Cache of materials */
  struct darray_material mtls;
  struct darray_material2 mtls2; /* Double sided materials */
  struct darray_matte mattes;
  struct darray_mirror mirrors;
  struct darray_thin_dielectric thin_dielectrics;

  /* Use to deferred the setup of the anchor targeted by a pivot */
  struct darray_tgtalias tgtaliases;

  /* Shape data */
  struct darray_shape shapes; /* Generic loaded shapes */
  struct darray_cuboid cuboids;
  struct darray_cylinder cylinders;
  struct darray_impgeom objs;
  struct darray_paraboloid parabols;
  struct darray_paraboloid parabolic_cylinders;
  struct darray_hyperboloid hyperbols;
  struct darray_plane planes;
  struct darray_sphere spheres;
  struct darray_impgeom stls;

  /* Geometries & objects */
  struct htable_yaml2sols yaml2geoms; /* Cache of geometries */
  struct darray_object objects;
  struct darray_geometry geometries;

  /* Sun. Note that only one sun is supported */
  const yaml_node_t* sun_key; /* yaml_node_t ptr used to spawn the sun */
  struct solparser_sun sun; /* The loaded sun */

  /* Entity */
  struct htable_yaml2sols yaml2entities; /* Cache of entities */
  struct htable_str2sols str2entities;
  struct darray_entity entities;

  /* Miscellaneous */
  struct darray_anchor anchors;
  struct darray_x_pivot x_pivots;
  struct darray_zx_pivot zx_pivots;

  ref_T ref;
  struct mem_allocator* allocator;
};

static res_T
parse_entity
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* entity,
   struct htable_str2sols* htable,
   struct solparser_entity_id* solent);

static res_T
parse_geometry
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* geometry,
   struct solparser_geometry_id* solgeom);

static res_T
parse_x_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* x_pivot,
   struct solparser_pivot_id* out_isolpivot);

static res_T
parse_zx_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* zx_pivot,
   struct solparser_pivot_id* out_isolpivot);

static res_T
parse_sun
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sun,
   struct solparser_sun** solsun);

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static INLINE void
log_err
  (const struct solparser* parser,
   const yaml_node_t* node,
   const char* fmt,
   ...)
{
  va_list vargs_list;
  ASSERT(parser && node && fmt);

  fprintf(stderr, "%s:%lu:%lu: ",
    str_cget(&parser->stream_name),
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
  va_start(vargs_list, fmt);
  vfprintf(stderr, fmt, vargs_list);
  va_end(vargs_list);
}

static INLINE void
log_node(const struct solparser* parser, const yaml_node_t* node)
{
  fprintf(stderr, "\tby %s:%lu:%lu\n",
    str_cget(&parser->stream_name),
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
}

static res_T
flush_deferred_target_aliases
  (struct solparser* parser,
   const yaml_node_t* node,
   const struct solparser_entity_id entity_id)
{
  const struct solparser_entity* entity;
  struct darray_char alias;
  size_t i, n;
  size_t prefix_len;
  res_T res = RES_OK;
  ASSERT(parser);

  darray_char_init(parser->allocator, &alias);

  if(!darray_tgtalias_size_get(&parser->tgtaliases))
    goto exit; /* No deferred target alias */

  /* Retrieve the entity referenced by the 'self' keyword */
  entity = solparser_get_entity(parser, entity_id);

  /* Copy the entity name */
  prefix_len = strlen(str_cget(&entity->name));
  res = darray_char_resize(&alias, prefix_len);
  if(res != RES_OK) {
    log_err(parser, node,
      "could not reserve the prefix of the targeted alias name.\n");
    goto error;
  }
  strncpy(darray_char_data_get(&alias), str_cget(&entity->name), prefix_len);

  n = darray_tgtalias_size_get(&parser->tgtaliases);
  FOR_EACH(i, 0, n) {
    const struct solparser_anchor* anchor;
    struct solparser_x_pivot* x_pivot;
    const struct target_alias* tgt;
    size_t ianchor;
    size_t len;

    tgt = darray_tgtalias_cdata_get(&parser->tgtaliases) + i;
    ASSERT(!strncmp((char*)tgt->alias->data.scalar.value, "self.", 5));

    /* Copy the anchor alias */
    len = strlen((char*)tgt->alias->data.scalar.value)
      - 4/*strlen(self)*/ + 1/*NULL char*/;
    res = darray_char_resize(&alias, prefix_len + len);
    if(res != RES_OK) {
      log_err(parser, node,
        "could not reserve the suffix of the targeted alias name.\n");
      goto error;
    }
    strcpy(darray_char_data_get(&alias) + prefix_len,
      (char*)tgt->alias->data.scalar.value+4);

    /* Retrieve the anchor */
    anchor = solparser_find_anchor(parser, darray_char_cdata_get(&alias));
    if(!anchor) {
      log_err(parser, tgt->alias, "undefined anchor `%s'.\n",
        tgt->alias->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }

    /* Define the targeted anchor of the pivot */
    x_pivot = darray_x_pivot_data_get(&parser->x_pivots) + tgt->pivot.i;
    ASSERT(x_pivot->target.type == SOLPARSER_TARGET_ANCHOR);
    ianchor = (size_t)(anchor - darray_anchor_cdata_get(&parser->anchors));
    ASSERT(ianchor < darray_anchor_size_get(&parser->anchors));
    x_pivot->target.data.anchor.i = ianchor;
  }

  darray_tgtalias_clear(&parser->tgtaliases);

exit:
  darray_char_release(&alias);
  return res;
error:
  goto exit;
}

/* Clean up loaded data */
static INLINE void
parser_clear(struct solparser* parser)
{
  ASSERT(parser);

  /* Materials */
  htable_yaml2sols_clear(&parser->yaml2mtls);
  darray_material_clear(&parser->mtls);
  darray_material2_clear(&parser->mtls2);
  darray_matte_clear(&parser->mattes);
  darray_mirror_clear(&parser->mirrors);
  darray_thin_dielectric_clear(&parser->thin_dielectrics);

  /* Deferred targeted anchors */
  darray_tgtalias_clear(&parser->tgtaliases);

  /* Shapes */
  darray_shape_clear(&parser->shapes);
  darray_cuboid_clear(&parser->cuboids);
  darray_cylinder_clear(&parser->cylinders);
  darray_impgeom_clear(&parser->objs);
  darray_paraboloid_clear(&parser->parabols);
  darray_paraboloid_clear(&parser->parabolic_cylinders);
  darray_hyperboloid_clear(&parser->hyperbols);
  darray_plane_clear(&parser->planes);
  darray_sphere_clear(&parser->spheres);
  darray_impgeom_clear(&parser->stls);

  /* Geometries */
  htable_yaml2sols_clear(&parser->yaml2geoms);
  darray_object_clear(&parser->objects);
  darray_geometry_clear(&parser->geometries);

  /* Sun */
  solparser_sun_clear(&parser->sun);
  parser->sun_key = 0;

  /* Entities */
  htable_yaml2sols_clear(&parser->yaml2entities);
  htable_str2sols_clear(&parser->str2entities);
  darray_entity_clear(&parser->entities);

  /* Miscellaneous */
  darray_anchor_clear(&parser->anchors);
  darray_x_pivot_clear(&parser->x_pivots);
  darray_zx_pivot_clear(&parser->zx_pivots);
}

static void
parser_release(ref_T* ref)
{
  struct solparser* parser;
  ASSERT(ref);

  parser = CONTAINER_OF(ref, struct solparser, ref);
  if(parser->parser_is_init) yaml_parser_delete(&parser->parser);
  str_release(&parser->stream_name);

  /* Materials */
  htable_yaml2sols_release(&parser->yaml2mtls);
  darray_material_release(&parser->mtls);
  darray_material2_release(&parser->mtls2);
  darray_matte_release(&parser->mattes);
  darray_mirror_release(&parser->mirrors);
  darray_thin_dielectric_release(&parser->thin_dielectrics);

  /* Deferred targeted anchors */
  darray_tgtalias_release(&parser->tgtaliases);

  /* Shapes */
  darray_shape_release(&parser->shapes);
  darray_cuboid_release(&parser->cuboids);
  darray_cylinder_release(&parser->cylinders);
  darray_impgeom_release(&parser->objs);
  darray_paraboloid_release(&parser->parabols);
  darray_paraboloid_release(&parser->parabolic_cylinders);
  darray_hyperboloid_release(&parser->hyperbols);
  darray_plane_release(&parser->planes);
  darray_sphere_release(&parser->spheres);
  darray_impgeom_release(&parser->stls);

  /* Geometries */
  htable_yaml2sols_release(&parser->yaml2geoms);
  darray_object_release(&parser->objects);
  darray_geometry_release(&parser->geometries);

  /* Sun */
  solparser_sun_release(&parser->sun);

  /* Entities */
  htable_yaml2sols_release(&parser->yaml2entities);
  htable_str2sols_release(&parser->str2entities);
  darray_entity_release(&parser->entities);

  /* Miscellaneous */
  darray_anchor_release(&parser->anchors);
  darray_x_pivot_release(&parser->x_pivots);
  darray_zx_pivot_release(&parser->zx_pivots);

  MEM_RM(parser->allocator, parser);
}

/*******************************************************************************
 * Miscellaneous parsing functions
 ******************************************************************************/
static res_T
parse_real
  (struct solparser* parser,
   const yaml_node_t* real,
   const double lower_bound,
   const double upper_bound,
   double* dst)
{
  res_T res = RES_OK;
  ASSERT(real && dst && lower_bound < upper_bound);

  if(real->type != YAML_SCALAR_NODE
  || !strlen((char*)real->data.scalar.value)) {
    log_err(parser, real, "expect a floating point number.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  res = cstr_to_double((char*)real->data.scalar.value, dst);
  if(res != RES_OK) {
    log_err(parser, real, "invalid floating point number `%s'.\n",
      real->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

  if(*dst < lower_bound || *dst > upper_bound) {
    double l = nextafter(lower_bound, -DBL_MAX);
    double u = nextafter(upper_bound, DBL_MAX);
    int l_excluded = (l == (double) (int) l);
    int u_excluded = (u == (double) (int) u);
    if(l_excluded && u_excluded) {
      log_err(parser, real, "%g is not in ]%g, %g[.\n",
        *dst, l, u);
    } else if(l_excluded) {
      log_err(parser, real, "%g is not in ]%g, %g].\n",
        *dst, l, upper_bound);
    } else if(u_excluded) {
      log_err(parser, real, "%g is not in [%g, %g[.\n",
        *dst, lower_bound, u);
    } else {
      log_err(parser, real, "%g is not in [%g, %g].\n",
        *dst, lower_bound, upper_bound);
    }
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_realX
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* realX,
   const double lower_bound,
   const double upper_bound,
   const size_t dim,
   double dst[])
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && realX && dst && dim > 0);

  if(realX->type != YAML_SEQUENCE_NODE) {
    log_err(parser, realX, "expect a sequence of 3 reals.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = realX->data.sequence.items.top - realX->data.sequence.items.start;
  if((size_t)n != dim) {
    log_err(parser, realX, "expect %lu reals while `%li' %s submitted.\n",
      (unsigned long)dim, n, n > 1 ? "are" : "is");
    res = RES_BAD_ARG;
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* real;
    real = yaml_document_get_node(doc, realX->data.sequence.items.start[i]);
    res = parse_real(parser, real, lower_bound, upper_bound, dst + i);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static FINLINE res_T
parse_real3
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* real3,
   const double lower_bound,
   const double upper_bound,
   double dst[3])
{
  return parse_realX(parser, doc, real3, lower_bound, upper_bound, 3, dst);
}

static FINLINE res_T
parse_real2
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* real2,
   const double lower_bound,
   const double upper_bound,
   double dst[2])
{
  return parse_realX(parser, doc, real2, lower_bound, upper_bound, 2, dst);
}

static res_T
parse_integer
  (struct solparser* parser,
   yaml_node_t* integer,
   const long lower_bound,
   const long upper_bound,
   long* dst)
{
  res_T res = RES_OK;
  ASSERT(integer && dst && lower_bound < upper_bound);

  if(integer->type != YAML_SCALAR_NODE
  || !strlen((char*)integer->data.scalar.value)) {
    log_err(parser, integer, "expect an integer.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  res = cstr_to_long((char*)integer->data.scalar.value, dst);
  if(res != RES_OK) {
    log_err(parser, integer, "invalid integer `%s'.\n",
      integer->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

  if(*dst < lower_bound || *dst > upper_bound) {
    log_err(parser, integer, "%li is not in [%li, %li].\n",
      *dst, lower_bound, upper_bound);
    res = RES_BAD_ARG;
    goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_string
  (struct solparser* parser,
   yaml_node_t* string,
   struct str* str)
{
  res_T res = RES_OK;
  ASSERT(string && str);

  if(string->type != YAML_SCALAR_NODE
  || !strlen((char*)string->data.scalar.value)) {
    log_err(parser, string, "expect a character string.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  res = str_set(str, (char*)string->data.scalar.value);
  if(res !=  RES_OK) {
    log_err(parser, string, "could not register the string `%s'.\n",
      string->data.scalar.value);
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_transform
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* transform,
   double translation[3],
   double rotation[3])
{
  enum { ROTATION, TRANSLATION };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && translation && rotation && transform);

  d3_splat(translation, 0);
  d3_splat(rotation, 0);

  if(transform->type != YAML_MAPPING_NODE) {
    log_err(parser, transform, "expect a mapping of transform parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = transform->data.mapping.pairs.top - transform->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, transform->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, transform->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect transform parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the transform `"Name"' is already defined.\n");  \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "translation")) {
      SETUP_MASK(TRANSLATION, "translation");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, translation);
    } else if(!strcmp((char*)key->data.scalar.value, "rotation")) {
      SETUP_MASK(ROTATION, "rotation");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, rotation);
    } else {
      log_err(parser, key, "unknown transform parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_spectrum_data
  (struct solparser* parser,
   yaml_document_t* doc,
   const double lower_bound,
   const double upper_bound,
   const yaml_node_t* sdata,
   struct solparser_spectrum_data* spectrum_data)
{
  enum { DATA, WAVELENGTH };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sdata && lower_bound < upper_bound && spectrum_data);

  if(sdata->type != YAML_MAPPING_NODE) {
    log_err(parser, sdata, "expect the definition of a spectrum data.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = sdata->data.mapping.pairs.top - sdata->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, sdata->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, sdata->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a spectrum data parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the `"Name"' of the spectrum data is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "data")) {
      SETUP_MASK(DATA, "data");
      res = parse_real(parser, val, lower_bound, upper_bound, &spectrum_data->data);
    } else if(!strcmp((char*)key->data.scalar.value, "wavelength")) {
      SETUP_MASK(WAVELENGTH, "wavelength");
      res = parse_real(parser, val, 0, DBL_MAX, &spectrum_data->wavelength);
    } else {
      log_err(parser, key, "unknown spectrum data parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, sdata,"the "Name" of the spectrum data is missing.\n");  \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(DATA, "data");
  CHECK_PARAM(WAVELENGTH, "wavelength");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_spectrum
  (struct solparser* parser,
   yaml_document_t* doc,
   const double lower_bound,
   const double upper_bound,
   const yaml_node_t* spectrum,
   struct darray_spectrum_data* data)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && spectrum && lower_bound < upper_bound && data);

  if(spectrum->type != YAML_SEQUENCE_NODE) {
    log_err(parser, spectrum, "expect a list of spectrum data.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = spectrum->data.sequence.items.top - spectrum->data.sequence.items.start;
  res = darray_spectrum_data_resize(data, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, spectrum, "could not allocate the list of spectrum data.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* sdata;
    struct solparser_spectrum_data* spectrum_data;

    sdata = yaml_document_get_node(doc, spectrum->data.sequence.items.start[i]);
    spectrum_data = darray_spectrum_data_data_get(data) + i;
    res = parse_spectrum_data
      (parser, doc, lower_bound, upper_bound, sdata, spectrum_data);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  darray_spectrum_data_clear(data);
  goto exit;
}

/*******************************************************************************
 * Material
 ******************************************************************************/
static res_T
parse_material_matte
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* matte,
   struct solparser_material_matte_id* out_imtl)
{
  enum { REFLECTIVITY };
  struct solparser_material_matte* mtl = NULL;
  size_t imtl = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && matte && out_imtl);

  if(matte->type != YAML_MAPPING_NODE) {
    log_err(parser, matte, "expect a mapping of matte material parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the matte material */
  imtl = darray_matte_size_get(&parser->mattes);
  res = darray_matte_resize(&parser->mattes, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, matte, "could not allocate the matte material.\n");
    goto error;
  }
  mtl = darray_matte_data_get(&parser->mattes) + imtl;

  n = matte->data.mapping.pairs.top - matte->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, matte->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, matte->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a matte material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      if(mask & BIT(REFLECTIVITY)) {
        log_err(parser, key, "the matte reflectivity is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(REFLECTIVITY);
      res = parse_real(parser, val, 0, 1, &mtl->reflectivity);
    } else {
      log_err(parser, key, "unknown matte parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }

  if(!(mask & BIT(REFLECTIVITY))) {
    log_err(parser, matte, "the matte reflectivity is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_matte_pop_back(&parser->mattes);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material_mirror
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* mirror,
   struct solparser_material_mirror_id* out_imtl)
{
  enum { REFLECTIVITY, ROUGHNESS };
  struct solparser_material_mirror* mtl = NULL;
  size_t imtl = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && mirror && out_imtl);

  if(mirror->type != YAML_MAPPING_NODE) {
    log_err(parser, mirror,
      "expect a mapping of mirror material attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the mirror material */
  imtl = darray_mirror_size_get(&parser->mirrors);
  res = darray_mirror_resize(&parser->mirrors, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, mirror, "could not allocate the mirror material.\n");
    goto error;
  }
  mtl = darray_mirror_data_get(&parser->mirrors) + imtl;

  n = mirror->data.mapping.pairs.top - mirror->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, mirror->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, mirror->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a mirror material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" of the mirror material is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "reflectivity")) {
      SETUP_MASK(REFLECTIVITY, "reflectivity");
      res = parse_real(parser, val, 0, 1, &mtl->reflectivity);
    } else if(!strcmp((char*)key->data.scalar.value, "roughness")) {
      SETUP_MASK(ROUGHNESS, "roughness");
      res = parse_real(parser, val, 0, 1, &mtl->roughness);
    } else {
      log_err(parser, key, "unknown mirror attribute `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, mirror, "the mirror "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(REFLECTIVITY, "reflectivity");
  CHECK_PARAM(ROUGHNESS, "roughness");
  #undef CHECK_PARAM

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_mirror_pop_back(&parser->mirrors);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material_thin_dielectric
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* thin,
   struct solparser_material_thin_dielectric_id* out_imtl)
{
  enum { ABSORPTION, REFRACTIVE_INDEX, THICKNESS };
  struct solparser_material_thin_dielectric* mtl = NULL;
  size_t imtl = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && thin && out_imtl);

  if(thin->type != YAML_MAPPING_NODE) {
    log_err(parser, thin,
      "expect a mapping of thin material attributes.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the thin dielectric material */
  imtl = darray_thin_dielectric_size_get(&parser->thin_dielectrics);
  res = darray_thin_dielectric_resize(&parser->thin_dielectrics, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, thin,
      "could not allocate the thin dielectric material.\n");
    goto error;
  }
  mtl = darray_thin_dielectric_data_get(&parser->thin_dielectrics) + imtl;

  n = thin->data.mapping.pairs.top - thin->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, thin->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, thin->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a thin dielectric material parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" of the thin dielectric material is already defined.\n"); \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "absorption")) {
      SETUP_MASK(ABSORPTION, "absorption");
      res = parse_real(parser, val, 0, 1, &mtl->absorption);
    } else if(!strcmp((char*)key->data.scalar.value, "refractive_index")) {
      SETUP_MASK(REFRACTIVE_INDEX, "refractive_index");
      res = parse_real
        (parser, val, nextafter(0, 1), DBL_MAX, &mtl->refractive_index);
    } else if(!strcmp((char*)key->data.scalar.value, "thickness")) {
      SETUP_MASK(THICKNESS, "thickness");
      res = parse_real(parser, val, 0, DBL_MAX, &mtl->thickness);
    } else {
      log_err(parser, key, "unknown thin dielectric parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, thin,                                                    \
        "the "Name" of the thin dielectric material is missing.\n");           \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(ABSORPTION, "absorption");
  CHECK_PARAM(REFRACTIVE_INDEX, "refractive_index");
  CHECK_PARAM(THICKNESS, "thickness");
  #undef CHECK_PARAM

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_thin_dielectric_pop_back(&parser->thin_dielectrics);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material_virtual(struct solparser* parser, yaml_node_t* virtual)
{
  res_T res = RES_OK;
  ASSERT(virtual);

  if(virtual->type != YAML_SCALAR_NODE
  || ((char*)virtual->data.scalar.value)[0] != '\0') {
    log_err(parser, virtual,
      "virtual materials can have a null scalar value only.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_material_descriptor
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* desc,
   struct solparser_material_id* out_imtl)
{
  enum { DESCRIPTOR };
  struct solparser_material* mtl = NULL;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  size_t* pimtl;
  size_t imtl = SIZE_MAX;
  res_T res = RES_OK;
  ASSERT(doc && desc && out_imtl);

  /* Check whether or not the YAML descriptor alias an already created Solstice
   * material */
  pimtl = htable_yaml2sols_find(&parser->yaml2mtls, &desc);
  if(pimtl) {
    imtl = *pimtl;
    goto exit;
  }

  /* Allocate the solstice material */
  imtl = darray_material_size_get(&parser->mtls);
  res = darray_material_resize(&parser->mtls, imtl + 1);
  if(res != RES_OK) {
    log_err(parser, desc, "could not allocate the material descriptor.\n");
    goto error;
  }
  mtl = darray_material_data_get(&parser->mtls) + imtl;

  if(desc->type != YAML_MAPPING_NODE) {
    log_err(parser, desc, "expect a material descriptor.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = desc->data.mapping.pairs.top - desc->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, desc->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, desc->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a material name.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the material "Name" is already defined.\n");     \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "matte")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_MATTE;
      res = parse_material_matte(parser, doc, val, &mtl->data.matte);
    } else if(!strcmp((char*)key->data.scalar.value, "mirror")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_MIRROR;
      res = parse_material_mirror(parser, doc, val, &mtl->data.mirror);
    } else if(!strcmp((char*)key->data.scalar.value, "thin_dielectric")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_THIN_DIELECTRIC;
      res = parse_material_thin_dielectric
        (parser, doc, val, &mtl->data.thin_dielectric);
    } else if(!strcmp((char*)key->data.scalar.value, "virtual")) {
      SETUP_MASK(DESCRIPTOR, "descriptor");
      mtl->type = SOLPARSER_MATERIAL_VIRTUAL;
      res = parse_material_virtual(parser, val);
    } else {
      log_err(parser, key, "unknown material descriptor `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  if(!(mask & BIT(DESCRIPTOR))) {
    log_err(parser, desc, "the material descriptor is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Cache the material */
  res = htable_yaml2sols_set(&parser->yaml2mtls, &desc, &imtl);
  if(res != RES_OK) {
    log_err(parser, desc, "could not register the material.\n");
    goto error;
  }

exit:
  out_imtl->i = imtl;
  return res;
error:
  if(mtl) {
    darray_material_pop_back(&parser->mtls);
    imtl = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_material
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* mtl,
   struct solparser_material_double_sided_id* out_imtl2)
{
  enum { FRONT, BACK };
  struct solparser_material_double_sided* mtl2 = NULL;
  size_t imtl2 = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && mtl && out_imtl2);

  if(mtl->type != YAML_MAPPING_NODE) {
    log_err(parser, mtl, "expect a material definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the double sided material */
  imtl2 = darray_material2_size_get(&parser->mtls2);
  res = darray_material2_resize(&parser->mtls2, imtl2 + 1);
  if(res != RES_OK) {
    log_err(parser, mtl, "could not allocate the material.\n");
    goto error;
  }
  mtl2 = darray_material2_data_get(&parser->mtls2) + imtl2;

  n = mtl->data.mapping.pairs.top - mtl->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, mtl->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, mtl->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key,
        "expect a material descriptor or a double sided material.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the "Name" material descriptor is already defined.\n");             \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "front")) {
      SETUP_MASK(FRONT, "front");
      res = parse_material_descriptor(parser, doc, val, &mtl2->front);
    } else if(!strcmp((char*)key->data.scalar.value, "back")) {
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(parser, doc, val, &mtl2->back);
    } else {
      SETUP_MASK(FRONT, "front");
      SETUP_MASK(BACK, "back");
      res = parse_material_descriptor(parser, doc, mtl, &mtl2->front);
      mtl2->back = mtl2->front;
      if(res != RES_OK) goto error; /* Discard log_node */
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                             \
    if(!(mask & BIT(Flag))) {                                                 \
      log_err(parser, mtl, "the "Name" material descriptor is missing.\n");   \
      res = RES_BAD_ARG;                                                      \
      goto error;                                                             \
    } (void)0
  CHECK_PARAM(FRONT, "front");
  CHECK_PARAM(BACK, "back");
  #undef CHECK_PARAM

exit:
  out_imtl2->i = imtl2;
  return res;
error:
  if(mtl2) {
    darray_material2_pop_back(&parser->mtls2);
    imtl2 = SIZE_MAX;
  }
  goto exit;
}

/*******************************************************************************
 * Clipping polygon
 ******************************************************************************/
static res_T
parse_clip_op
  (struct solparser* parser,
   const yaml_node_t* op,
   enum solparser_clip_op* clip_op)
{
  res_T res = RES_OK;
  ASSERT(op && clip_op);

  if(op->type != YAML_SCALAR_NODE) {
    log_err(parser, op, "expect a clipping operation.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)op->data.scalar.value, "AND")) {
    *clip_op = SOLPARSER_CLIP_OP_AND;
  } else if(!strcmp((char*)op->data.scalar.value, "SUB")) {
    *clip_op = SOLPARSER_CLIP_OP_SUB;
  } else {
    log_err(parser, op, "unknown clipping operation `%s'.\n",
      op->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_vertices
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* vertices,
   struct darray_double* coords)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && vertices && coords);

  if(vertices->type != YAML_SEQUENCE_NODE) {
    log_err(parser, vertices, "expect a list of vertices.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = vertices->data.sequence.items.top - vertices->data.sequence.items.start;
  if(n < 3) {
    log_err(parser, vertices, "expect at least 3 vertices.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  res = darray_double_resize(coords, (size_t)n*2/*#coords per vertex*/);
  if(res != RES_OK) {
    log_err(parser, vertices, "could not allocate the array of vertices.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* vertex;
    double* real2 = darray_double_data_get(coords) + i*2/*#coords per vertex*/;

    vertex = yaml_document_get_node(doc, vertices->data.sequence.items.start[i]);
    res = parse_real2(parser, doc, vertex, -DBL_MAX, DBL_MAX, real2);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  darray_double_clear(coords);
  goto exit;
}

static res_T
parse_polyclip
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* polyclip,
   struct solparser_polyclip* clip)
{
  enum { OPERATION, VERTICES };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && polyclip && clip);

  if(polyclip->type != YAML_MAPPING_NODE) {
    log_err(parser, polyclip,
      "expect a mapping of clipping polygon parameters.\n");
    res = RES_OK;
    goto error;
  }

  n = polyclip->data.mapping.pairs.top - polyclip->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, polyclip->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, polyclip->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a clipping polygon parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the clipping polygon parameter `"Name"' is already defined.\n");    \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "operation")) {
      SETUP_MASK(OPERATION, "operation");
      res = parse_clip_op(parser, val, &clip->op);
    } else if(!strcmp((char*)key->data.scalar.value, "vertices")) {
      SETUP_MASK(VERTICES, "vertices");
      res = parse_vertices(parser, doc, val, &clip->vertices);
    } else {
      log_err(parser, key, "unknown clipping polygon parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, polyclip,                                                \
        "the clipping polygon parameter `"Name"' is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(OPERATION, "operation");
  CHECK_PARAM(VERTICES, "vertices");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_clip
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* clip,
   struct darray_polyclip* polyclips)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && clip && polyclips);

  if(clip->type != YAML_SEQUENCE_NODE) {
    log_err(parser, clip, "expect a list of clipping polygons.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = clip->data.sequence.items.top - clip->data.sequence.items.start;

  /* Allocate the clipping polygons */
  res = darray_polyclip_resize(polyclips, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, clip, "could not allocate the list of clipping polygons.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    yaml_node_t* node;
    struct solparser_polyclip* polyclip = darray_polyclip_data_get(polyclips) + i;

    node = yaml_document_get_node(doc, clip->data.sequence.items.start[i]);
    res = parse_polyclip(parser, doc, node, polyclip);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Shapes
 ******************************************************************************/
static res_T
parse_cuboid
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* cuboid,
   struct solparser_shape_cuboid_id* out_ishape)
{
  enum { SIZE };
  struct solparser_shape_cuboid* shape = NULL;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && cuboid && out_ishape);

  if(cuboid->type != YAML_MAPPING_NODE) {
    log_err(parser, cuboid, "expect a mapping of cuboid parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a cuboid */
  ishape = darray_cuboid_size_get(&parser->cuboids);
  res = darray_cuboid_resize(&parser->cuboids, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, cuboid, "could not allocate the cuboid shape.\n");
    goto exit;
  }
  shape = darray_cuboid_data_get(&parser->cuboids) + ishape;

  n = cuboid->data.mapping.pairs.top - cuboid->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, cuboid->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, cuboid->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect cuboid parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "size")) {
      if(mask & BIT(SIZE)) {
        log_err(parser, key, "the cuboid size is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(SIZE);
      res = parse_real3(parser, doc, val, nextafter(0, 1), DBL_MAX, shape->size);
    } else {
      log_err(parser, key, "unknown cuboid parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }

  if(!(mask & BIT(SIZE))) {
    log_err(parser, cuboid, "the size of the cuboid is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  out_ishape->i = ishape;
  return res;
error:
  if(shape) {
    darray_cuboid_pop_back(&parser->cuboids);
    ishape = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_cylinder
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* cylinder,
   struct solparser_shape_cylinder_id* out_ishape)
{
  enum { HEIGHT, RADIUS, SLICES };
  struct solparser_shape_cylinder* shape = NULL;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && cylinder && out_ishape);

  if(cylinder->type != YAML_MAPPING_NODE) {
    log_err(parser, cylinder, "expect a mapping of cylinder parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a cylinder */
  ishape = darray_cylinder_size_get(&parser->cylinders);
  res = darray_cylinder_resize(&parser->cylinders, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, cylinder, "could not alocate the cylinder shape.\n");
    goto exit;
  }
  shape = darray_cylinder_data_get(&parser->cylinders) + ishape;

  n = cylinder->data.mapping.pairs.top - cylinder->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, cylinder->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, cylinder->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect cylinder parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the cylinder parameter `"Name"' is already defined.\n");            \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "height")) {
      SETUP_MASK(HEIGHT, "height");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &shape->height);
    } else if(!strcmp((char*)key->data.scalar.value, "radius")) {
      SETUP_MASK(RADIUS, "radius");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &shape->radius);
    } else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 4, 4096, &shape->nslices);
    } else {
      log_err(parser, key, "unknown cylinder parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, cylinder,                                                \
        "the cylinder parameter `"Name"' is missing.\n");                      \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(HEIGHT, "height");
  CHECK_PARAM(RADIUS, "radius");
  #undef CHECK_PARAM

  #define DEFAULT_PARAM(Flag, Ptr, Value)                                      \
    if(!(mask & BIT(Flag))) {                                                  \
      *(Ptr) = Value;                                                          \
    } (void)0
  DEFAULT_PARAM(SLICES, &shape->nslices, 16);
  #undef DEFAULT_PARAM

exit:
  out_ishape->i = ishape;
  return res;
error:
  if(shape) {
    darray_cylinder_pop_back(&parser->cylinders);
    ishape = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_imported_geometry
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* geom,
   const enum solparser_shape_type type,
   struct solparser_shape_imported_geometry_id* out_ishape)
{
  enum { PATH };
  struct solparser_shape_imported_geometry* shape = NULL;
  size_t ishape = SIZE_MAX;
  const char* name;
  struct darray_impgeom* impgeoms;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && geom && out_ishape);

  switch(type) {
    case SOLPARSER_SHAPE_OBJ: name = "obj"; impgeoms = &parser->objs; break;
    case SOLPARSER_SHAPE_STL: name = "stl"; impgeoms = &parser->stls; break;
    default: FATAL("Unreachable code.\n"); break;
  }

  if(geom->type != YAML_MAPPING_NODE) {
    log_err(parser, geom, "expect a mapping of %s parameters.\n", name);
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate an imported geometry */
  ishape = darray_impgeom_size_get(impgeoms);
  res = darray_impgeom_resize(impgeoms, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, geom, "could not allocate the %s shape.\n", name);
    goto error;
  }
  shape = darray_impgeom_data_get(impgeoms) + ishape;

  n = geom->data.mapping.pairs.top - geom->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, geom->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, geom->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect %s parameters.\n", name);
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "path")) {
      if(mask & BIT(PATH)) {
        log_err(parser, key, "the %s path is already defined.\n", name);
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(PATH);
      res = parse_string(parser, val, &shape->filename);
    } else {
      log_err(parser, key, "unknown %s parameter `%s'.\n",
        name, key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }

  if(!(mask & BIT(PATH))) {
    log_err(parser, geom, "the path of the %s geometry is missing.\n", name);
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  out_ishape->i = ishape;
  return res;
error:
  if(shape) {
    darray_impgeom_pop_back(impgeoms);
    ishape = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_paraboloid
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* paraboloid,
   const enum solparser_shape_type type,
   struct solparser_shape_paraboloid_id* out_ishape)
{
  enum { CLIP, FOCAL };
  struct solparser_shape_paraboloid* shape = NULL;
  struct darray_paraboloid* paraboloids;
  const char* name;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && paraboloid && out_ishape);

  switch(type) {
    case SOLPARSER_SHAPE_PARABOL:
      name = "parabol";
      paraboloids = &parser->parabols;
      break;
    case SOLPARSER_SHAPE_PARABOLIC_CYLINDER:
      name = "parabolic cylinder";
      paraboloids = &parser->parabolic_cylinders;
      break;
    default: FATAL("Unreachable code.\n"); break;
  }

  if(paraboloid->type != YAML_MAPPING_NODE) {
    log_err(parser, paraboloid, "expect a mapping of %s parameters.\n", name);
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a paraboloid shape */
  ishape = darray_paraboloid_size_get(paraboloids);
  res = darray_paraboloid_resize(paraboloids, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, paraboloid, "could not allocate the %s shape.\n", name);
    goto error;
  }
  shape = darray_paraboloid_data_get(paraboloids) + ishape;

  n = paraboloid->data.mapping.pairs.top - paraboloid->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, paraboloid->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, paraboloid->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect %s parameters.\n", name);
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the %s parameter `"Name"' is already defined.\n", name);            \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "clip")) {
      SETUP_MASK(CLIP, "clip");
      res = parse_clip(parser, doc, val, &shape->polyclips);
    } else if(!strcmp((char*)key->data.scalar.value, "focal")) {
      SETUP_MASK(FOCAL, "focal");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &shape->focal);
    } else {
      log_err(parser, key, "unknown %s parameter `%s'.\n",
        name, key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }
  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, paraboloid,                                              \
        "the %s parameter `"Name"' is missing.\n", name);                      \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(CLIP, "clip");
  CHECK_PARAM(FOCAL, "focal");
  #undef CHECK_PARAM

exit:
  out_ishape->i = ishape;
  return res;
error:
  if(shape) {
    darray_paraboloid_pop_back(paraboloids);
    ishape = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_focals_description
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* desc,
   struct hyperboloid_focals* focals)
{
  enum { REAL, IMAGE };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && desc && focals);

  if (desc->type != YAML_MAPPING_NODE) {
    log_err(parser, desc, "expect a mapping of focal parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = desc->data.mapping.pairs.top - desc->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, desc->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, desc->data.mapping.pairs.start[i].value);
    if (key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect focal parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the focal parameter `"Name"' is already defined.\n");               \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if (!strcmp((char*) key->data.scalar.value, "real")) {
      SETUP_MASK(REAL, "real");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &focals->real);
    }
    else if (!strcmp((char*) key->data.scalar.value, "image")) {
      SETUP_MASK(IMAGE, "image");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &focals->image);
    }
    if (res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }
  #undef SETUP_MASK
  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, desc,                                                    \
        "the focal parameter `"Name"' is missing.\n");                         \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(REAL, "real");
  CHECK_PARAM(IMAGE, "image");
  #undef CHECK_PARAM

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_hyperboloid
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* hyperboloid,
   struct solparser_shape_hyperboloid_id* out_ishape)
{
  enum { CLIP, FOCAL };
  struct solparser_shape_hyperboloid* shape = NULL;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && hyperboloid && out_ishape);

  if (hyperboloid->type != YAML_MAPPING_NODE) {
    log_err(parser, hyperboloid, "expect a mapping of hyperbol parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a hyperboloid shape */
  ishape = darray_hyperboloid_size_get(&parser->hyperbols);
  res = darray_hyperboloid_resize(&parser->hyperbols, ishape + 1);
  if (res != RES_OK) {
    log_err(parser, hyperboloid, "could not allocate the hyperbol shape.\n");
    goto error;
  }
  shape = darray_hyperboloid_data_get(&parser->hyperbols) + ishape;

  n = hyperboloid->data.mapping.pairs.top - hyperboloid->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, hyperboloid->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, hyperboloid->data.mapping.pairs.start[i].value);
    if (key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect hyperbol parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the hyperbol parameter `"Name"' is already defined.\n");            \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if (!strcmp((char*) key->data.scalar.value, "clip")) {
      SETUP_MASK(CLIP, "clip");
      res = parse_clip(parser, doc, val, &shape->polyclips);
    }
    else if (!strcmp((char*) key->data.scalar.value, "focals")) {
      SETUP_MASK(FOCAL, "focals");
      res = parse_focals_description(parser, doc, val, &shape->focals);
    }
    else {
      log_err(parser, key, "unknown hyperbol parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if (res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }
  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, hyperboloid,                                             \
        "the hyperbol parameter `"Name"' is missing.\n");                      \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(CLIP, "clip");
  CHECK_PARAM(FOCAL, "focals");
  #undef CHECK_PARAM

exit :
  out_ishape->i = ishape;
  return res;
error:
  if (shape) {
    darray_hyperboloid_pop_back(&parser->hyperbols);
    ishape = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_plane
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* plane,
   struct solparser_shape_plane_id* out_ishape)
{
  enum { CLIP };
  struct solparser_shape_plane* shape = NULL;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && plane && out_ishape);

  if(plane->type != YAML_MAPPING_NODE) {
    log_err(parser, plane, "expect a mapping of plane parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a plane shape */
  ishape = darray_plane_size_get(&parser->planes);
  res = darray_plane_resize(&parser->planes, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, plane, "could not allocate the plane shape.\n");
    goto error;
  }
  shape = darray_plane_data_get(&parser->planes) + ishape;

  n = plane->data.mapping.pairs.top - plane->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, plane->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, plane->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect plane parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "clip")) {
      if(mask & BIT(CLIP)) {
        log_err(parser, key, "the plane clipping is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(CLIP);
      res = parse_clip(parser, doc, val, &shape->polyclips);
    } else {
      log_err(parser, key, "unknown plane parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }
  if(!(mask & BIT(CLIP))) {
    log_err(parser, plane, "the plane parameter `clip' is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  out_ishape->i = ishape;
  return res;
error:
  if(shape) {
    darray_plane_pop_back(&parser->planes);
    ishape = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_sphere
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sphere,
   struct solparser_shape_sphere_id* out_ishape)
{
  enum { RADIUS, SLICES };
  struct solparser_shape_sphere* shape = NULL;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sphere && out_ishape);

  if(sphere->type != YAML_MAPPING_NODE) {
    log_err(parser, sphere, "expect a mapping of sphere parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a shpere shape */
  ishape = darray_sphere_size_get(&parser->spheres);
  res = darray_sphere_resize(&parser->spheres, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, sphere, "could not allocate the sphere shape.\n");
    goto error;
  }
  shape = darray_sphere_data_get(&parser->spheres) + ishape;

  n = sphere->data.mapping.pairs.top - sphere->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, sphere->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, sphere->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect sphere parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the sphere parameter `"Name"' is already defined.\n");              \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "radius")) {
      SETUP_MASK(RADIUS, "radius");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &shape->radius);
    } else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 4, 4096, &shape->nslices);
    } else {
      log_err(parser, key, "unknown sphere parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, sphere,                                                  \
        "the sphere parameter `"Name"' is missing.\n");                        \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(RADIUS, "radius");
  #undef CHECK_PARAM

  #define DEFAULT_PARAM(Flag, Ptr, Value)                                      \
    if(!(mask & BIT(Flag))) {                                                  \
      *(Ptr) = Value;                                                          \
    } (void)0
  DEFAULT_PARAM(SLICES, &shape->nslices, 16);
  #undef DEFAULT_PARAM

exit:
  out_ishape->i = ishape;
  return res;
error:
  if(shape) {
    darray_sphere_pop_back(&parser->spheres);
    ishape = SIZE_MAX;
  }
  goto exit;
}

/*******************************************************************************
 * Geometry
 ******************************************************************************/
static res_T
parse_object
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* object,
   struct solparser_object_id* out_iobj)
{
  enum { MATERIAL, SHAPE, TRANSFORM };
  struct solparser_object* obj = NULL;
  struct solparser_shape* shape = NULL;
  size_t iobj = SIZE_MAX;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && object && out_iobj);

  if(object->type != YAML_MAPPING_NODE) {
    log_err(parser, object, "expect an object definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate an object */
  iobj = darray_object_size_get(&parser->objects);
  res = darray_object_resize(&parser->objects, iobj + 1);
  if(res != RES_OK) {
    log_err(parser, object, "could not allocate the object.\n");
    goto error;
  }
  obj = darray_object_data_get(&parser->objects) + iobj;

  /* Allocate a shape */
  ishape = darray_shape_size_get(&parser->shapes);
  res = darray_shape_resize(&parser->shapes, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, object, "could not allocate the object shape.\n");
    goto error;
  }
  shape = darray_shape_data_get(&parser->shapes) + ishape;
  obj->shape.i = ishape;

  /* Setup default object transformation */
  d3_splat(obj->translation, 0);
  d3_splat(obj->rotation, 0);

  n = object->data.mapping.pairs.top - object->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, object->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, object->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an object parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the object "Name" is already defined.\n");                          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "material")) {
      SETUP_MASK(MATERIAL, "material");
      res = parse_material(parser, doc, val, &obj->mtl2);
    } else if(!strcmp((char*)key->data.scalar.value, "cuboid")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_CUBOID;
      res = parse_cuboid(parser, doc, val, &shape->data.cuboid);
    } else if(!strcmp((char*)key->data.scalar.value, "cylinder")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_CYLINDER;
      res = parse_cylinder(parser, doc, val, &shape->data.cylinder);
    } else if(!strcmp((char*)key->data.scalar.value, "obj")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_OBJ;
      res = parse_imported_geometry
        (parser, doc, val, shape->type, &shape->data.obj);
    } else if(!strcmp((char*)key->data.scalar.value, "parabol")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_PARABOL;
      res = parse_paraboloid
        (parser, doc, val, shape->type, &shape->data.parabol);
    } else if(!strcmp((char*)key->data.scalar.value, "parabolic-cylinder")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_PARABOLIC_CYLINDER;
      res = parse_paraboloid
        (parser, doc, val, shape->type, &shape->data.parabolic_cylinder);
    }
    else if (!strcmp((char*) key->data.scalar.value, "hyperbol")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_HYPERBOL;
      res = parse_hyperboloid(parser, doc, val, &shape->data.hyperbol);
    } else if(!strcmp((char*)key->data.scalar.value, "plane")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_PLANE;
      res = parse_plane(parser, doc, val, &shape->data.plane);
    } else if(!strcmp((char*)key->data.scalar.value, "sphere")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_SPHERE;
      res = parse_sphere(parser, doc, val, &shape->data.sphere);
    } else if(!strcmp((char*)key->data.scalar.value, "stl")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_STL;
      res = parse_imported_geometry
        (parser, doc, val, shape->type, &shape->data.stl);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform(parser, doc, val, obj->translation, obj->rotation);
    } else {
      log_err(parser, key, "unknown object parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, object, "the object "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(MATERIAL, "material");
  CHECK_PARAM(SHAPE, "shape");
  #undef CHECK_PARAM

exit:
  out_iobj->i = iobj;
  return res;
error:
  if(obj) {
    if(shape) darray_shape_pop_back(&parser->shapes);
    darray_object_pop_back(&parser->objects);
    obj = NULL;
  }
  goto exit;
}

static res_T
parse_geometry
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* geometry,
   struct solparser_geometry_id* out_isolgeom)
{
  struct solparser_geometry* solgeom = NULL;
  size_t* pisolgeom;
  size_t isolgeom = SIZE_MAX;
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && geometry && out_isolgeom);

  if(geometry->type != YAML_SEQUENCE_NODE) {
    log_err(parser, geometry, "expect a list of objects.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Check whether or not the YAML descriptor alias an already created Solstice
   * geometry */
  pisolgeom = htable_yaml2sols_find(&parser->yaml2geoms, &geometry);
  if(pisolgeom) {
    isolgeom = *pisolgeom;
    goto exit;
  }

  /* Allocate the geometry */
  isolgeom = darray_geometry_size_get(&parser->geometries);
  res = darray_geometry_resize(&parser->geometries, isolgeom + 1);
  if(res != RES_OK) {
    log_err(parser, geometry, "could not allocate the geometry.\n");
    goto error;
  }
  solgeom = darray_geometry_data_get(&parser->geometries) + isolgeom;

  n = geometry->data.sequence.items.top - geometry->data.sequence.items.start;
  res = darray_object_id_resize(&solgeom->objects, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, geometry, "could not allocate the objects list.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    struct solparser_object_id* obj_id;
    yaml_node_t* obj;

    obj_id = darray_object_id_data_get(&solgeom->objects) + i;
    obj = yaml_document_get_node(doc, geometry->data.sequence.items.start[i]);
    res = parse_object(parser, doc, obj, obj_id);
    if(res != RES_OK) goto error;
  }

  /* Cache the geometry */
  res = htable_yaml2sols_set(&parser->yaml2geoms, &geometry, &isolgeom);
  if(res != RES_OK) {
    log_err(parser, geometry, "could not register the geometry.\n");
    goto error;
  }

exit:
  out_isolgeom->i = isolgeom;
  return res;
error:
  if(solgeom) {
    darray_geometry_pop_back(&parser->geometries);
    isolgeom = SIZE_MAX;
  }
  goto exit;
}


/*******************************************************************************
 * Entity
 ******************************************************************************/
static res_T
entity_register_name
  (struct solparser* parser,
   const yaml_node_t* entity,
   struct htable_str2sols* htable,
   const size_t isolent)
{
  struct solparser_entity* solent;
  size_t* pisolent;
  res_T res = RES_OK;
  ASSERT(parser && htable);
  ASSERT(isolent < darray_entity_size_get(&parser->entities));

  solent = darray_entity_data_get(&parser->entities) + isolent;

  pisolent = htable_str2sols_find(htable, &solent->name);
  if(pisolent) {
    log_err(parser, entity,
      "an entity with the name `%s' is already defined in the current context.\n",
      str_cget(&solent->name));
    return RES_BAD_ARG;
  }

  res = htable_str2sols_set(htable, &solent->name, &isolent);
  if(res != RES_OK) {
    log_err(parser, entity, "could not register the entity.\n");
    return res;
  }
  return RES_OK;
}

static res_T
anchor_register_name
  (struct solparser* parser,
   const yaml_node_t* anchor,
   struct htable_str2sols* htable,
   const size_t isolanchor)
{
  struct solparser_anchor* solanchor;
  size_t* pisolanchor;
  res_T res = RES_OK;
  ASSERT(parser && htable);
  ASSERT(isolanchor < darray_anchor_size_get(&parser->anchors));

  solanchor = darray_anchor_data_get(&parser->anchors) + isolanchor;

  pisolanchor = htable_str2sols_find(htable, &solanchor->name);
  if(pisolanchor) {
    log_err(parser, anchor,
      "an anchor with the name `%s' is already defined in the cunrrent context.\n",
      str_cget(&solanchor->name));
    return RES_BAD_ARG;
  }

  res = htable_str2sols_set(htable, &solanchor->name, &isolanchor);
  if(res != RES_OK) {
    log_err(parser, anchor, "could not register the anchor.\n");
    return res;
  }
  return RES_OK;
}

static res_T
parse_identifier_string
  (struct solparser* parser,
   yaml_node_t* name,
   struct str* str)
{
  res_T res = RES_OK;
  ASSERT(parser && name && str);

  res = parse_string(parser, name, str);
  if(res != RES_OK) goto error;

  if(strchr(str_cget(str), '.')) {
    log_err(parser, name, "invalid character `.' in the name `%s'.\n",
      str_cget(str));
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_anchor
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* anchor,
   struct htable_str2sols* htable,
   struct solparser_anchor_id* out_isolanchor)
{
  enum { NAME, POSITION };
  struct solparser_anchor* solanchor = NULL;
  size_t isolanchor = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(parser && anchor && out_isolanchor);

  if(anchor->type != YAML_MAPPING_NODE) {
    log_err(parser, anchor, "expect an anchor definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the anchor */
  isolanchor = darray_anchor_size_get(&parser->anchors);
  res = darray_anchor_resize(&parser->anchors, isolanchor + 1);
  if(res != RES_OK) {
    log_err(parser, anchor, "could not allocate the anchor.\n");
    goto error;
  }
  solanchor = darray_anchor_data_get(&parser->anchors) + isolanchor;

  n = anchor->data.mapping.pairs.top - anchor->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, anchor->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, anchor->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an anchor attribute.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the anchor "Name" is already defined.\n");       \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "name")) {
      SETUP_MASK(NAME, "name");
      res = parse_identifier_string(parser, val, &solanchor->name);
    } else if(!strcmp((char*)key->data.scalar.value, "position")) {
      SETUP_MASK(POSITION, "position description");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, solanchor->position);
    }
    else if (!strcmp((char*) key->data.scalar.value, "hyperboloid_image_focals"))
    {
      struct hyperboloid_focals focals;
      SETUP_MASK(POSITION, "position description");
      res = parse_focals_description(parser, doc, val, &focals);
      if (res != RES_OK) goto error;
      d3(solanchor->position, 0, 0, focals.image);
    } else {
      log_err(parser, key, "unknown anchor parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, anchor, "the anchor "Name" is missing.\n");              \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(NAME, "name");
  CHECK_PARAM(POSITION, "position description");
  #undef CHECK_PARAM

  res = anchor_register_name(parser, anchor, htable, isolanchor);
  if(res != RES_OK) goto error;

exit:
  out_isolanchor->i = isolanchor;
  return res;
error:
  if(solanchor) {
    darray_anchor_pop_back(&parser->anchors);
    isolanchor = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_anchors
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* anchors,
   struct htable_str2sols* htable,
   struct darray_anchor_id* solanchors)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(parser && anchors);

  if(anchors->type != YAML_SEQUENCE_NODE) {
    log_err(parser, anchors, "expect a list of anchors.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = anchors->data.sequence.items.top - anchors->data.sequence.items.start;
  res = darray_anchor_id_resize(solanchors, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, anchors, "could not allocate the anchors list.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    struct solparser_anchor_id* anchor_id;
    yaml_node_t* anchor;

    anchor_id = darray_anchor_id_data_get(solanchors)+i;
    anchor = yaml_document_get_node(doc, anchors->data.sequence.items.start[i]);
    res = parse_anchor(parser, doc, anchor, htable, anchor_id);
    if(res != RES_OK) goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_children
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* children,
   struct htable_str2sols* htable,
   struct darray_child_id* entities)
{
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(parser && children && htable && entities);

  if(children->type != YAML_SEQUENCE_NODE) {
    log_err(parser, children, "expect a list of entities.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = children->data.sequence.items.top - children->data.sequence.items.start;
  res = darray_child_id_resize(entities, (size_t)n);
  if(res != RES_OK) {
    log_err(parser, children, "could not allocate the children list.\n");
    goto error;
  }

  FOR_EACH(i, 0, n) {
    struct solparser_entity_id* entity_id = darray_child_id_data_get(entities) + i;
    yaml_node_t* child;

    child = yaml_document_get_node(doc, children->data.sequence.items.start[i]);
    res = parse_entity(parser, doc, child, htable, entity_id);
    if(res != RES_OK) goto error;
  }

exit:
  return res;
error:
  darray_child_id_clear(entities);
  goto exit;
}

res_T
parse_entity
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* entity,
   struct htable_str2sols* htable,
   struct solparser_entity_id* out_isolent)
{
  enum { ANCHORS, CHILDREN, DATA, NAME, TRANSFORM, PRIMARY };
  struct solparser_entity solent;
  struct solparser_entity* psolent;
  const size_t *pisolent;
  size_t isolent = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && entity && htable && out_isolent);

  solparser_entity_init(parser->allocator, &solent);

  pisolent = htable_yaml2sols_find(&parser->yaml2entities, &entity);
  if(pisolent) {
    isolent = *pisolent;
    res = entity_register_name(parser, entity, htable, *pisolent);
    if(res != RES_OK) goto error;
    goto exit;
  }

  if(entity->type != YAML_MAPPING_NODE) {
    log_err(parser, entity, "expect an entity definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the entity but *DO NOT* retrieve a pointer onto it since the
   * allocation of its children may update its memory location. Use the "on
   * stack" entity `solent' instead. */
  isolent = darray_entity_size_get(&parser->entities);
  res = darray_entity_resize(&parser->entities, isolent + 1);
  if(res != RES_OK) {
    log_err(parser, entity, "could not allocate the entity.\n");
    goto error;
  }

  n = entity->data.mapping.pairs.top - entity->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, entity->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, entity->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect an entity attribute.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the entity "Name" is already defined.\n");                          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "anchors")) {
      SETUP_MASK(ANCHORS, "anchors");
      res = parse_anchors
        (parser, doc, val, &solent.str2anchors, &solent.anchors);
    } else if(!strcmp((char*)key->data.scalar.value, "children")) {
      SETUP_MASK(CHILDREN, "children");
      res = parse_children
        (parser, doc, val, &solent.str2children, &solent.children);
    } else if(!strcmp((char*)key->data.scalar.value, "geometry")) {
      SETUP_MASK(DATA, "data");
      solent.type = SOLPARSER_ENTITY_GEOMETRY;
      res = parse_geometry(parser, doc, val, &solent.data.geometry);
    } else if(!strcmp((char*)key->data.scalar.value, "name")) {
      SETUP_MASK(NAME, "name");
      res = parse_identifier_string(parser, val, &solent.name);
      if (!strcmp(str_get(&solent.name), "self")) {
        /* self is a reserved keyword */
        log_err(parser, key, "Reserved keywords cannot be used as names: %s.\n",
          str_get(&solent.name));
        res = RES_BAD_ARG;
        goto error;
      }
    } else if(!strcmp((char*)key->data.scalar.value, "x_pivot")) {
      SETUP_MASK(DATA, "data");
      solent.type = SOLPARSER_ENTITY_X_PIVOT;
      res = parse_x_pivot(parser, doc, val, &solent.data.x_pivot);
    } else if(!strcmp((char*) key->data.scalar.value, "zx_pivot")) {
      SETUP_MASK(DATA, "data");
      solent.type = SOLPARSER_ENTITY_ZX_PIVOT;
      res = parse_zx_pivot(parser, doc, val, &solent.data.zx_pivot);
    } else if(!strcmp((char*)key->data.scalar.value, "transform")) {
      SETUP_MASK(TRANSFORM, "transform");
      res = parse_transform
        (parser, doc, val, solent.translation, solent.rotation);
    } else if(!strcmp((char*) key->data.scalar.value, "primary")) {
      long tmp;
      SETUP_MASK(PRIMARY, "primary");
      res = parse_integer(parser, val, 0, 1, &tmp);
      solent.primary = (int)tmp;
    } else {
      log_err(parser, key, "unknown entity parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  if(!(mask & BIT(DATA))) {
    solent.type = SOLPARSER_ENTITY_EMPTY;
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, entity, "the entity "Name" parameter is missing.\n");    \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(NAME, "name");
  if(solent.type == SOLPARSER_ENTITY_GEOMETRY) {
    CHECK_PARAM(PRIMARY, "primary");
  } else if(mask & BIT(PRIMARY)) {
    log_err(parser, entity,
      "the entity primary parameter is invalid in this context.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  #undef CHECK_PARAM

  psolent = darray_entity_data_get(&parser->entities) + isolent;
  res = solparser_entity_copy_and_clear(psolent, &solent);
  if(res != RES_OK) {
    log_err(parser, entity,
      "could not copy the loaded entity into the parser data structures.\n");
    goto error;
  }
  res = entity_register_name(parser, entity, htable, isolent);
  if(res != RES_OK) goto error;

  res = htable_yaml2sols_set(&parser->yaml2entities, &entity, &isolent);
  if(res != RES_OK) {
    log_err(parser, entity, "could not register the entity.\n");
    goto error;
  }

exit:
  solparser_entity_release(&solent);
  out_isolent->i = isolent;
  return res;
error:
  if(isolent != SIZE_MAX) {
    htable_str2sols_erase(htable, &solent.name);
    darray_entity_pop_back(&parser->entities);
    isolent = SIZE_MAX;
  }
  goto exit;
}

/*******************************************************************************
 * Pivot
 ******************************************************************************/
static res_T
parse_anchor_alias
  (struct solparser* parser,
   const yaml_node_t* alias,
   const struct solparser_pivot_id pivot,
   struct solparser_anchor_id* out_ianchor)
{
  const struct solparser_anchor* anchor = NULL;
  intptr_t ianchor = INTPTR_MAX;
  res_T res = RES_OK;
  ASSERT(parser && alias && out_ianchor);

  if(alias->type != YAML_SCALAR_NODE) {
    log_err(parser, alias, "expect an anchor idententifier.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strncmp((char*)alias->data.scalar.value, "self.", 5)) {
    struct target_alias tgt;
    tgt.pivot = pivot;
    tgt.alias = alias;
    res = darray_tgtalias_push_back(&parser->tgtaliases, &tgt);
    if(res != RES_OK) {
      log_err(parser, alias, "could not register the anchor alias.\n");
      goto error;
    }
  } else {
    anchor = solparser_find_anchor(parser, (char*)alias->data.scalar.value);
    if(!anchor) {
      log_err(parser, alias, "undefined anchor `%s'.\n",
        alias->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }

    ianchor = anchor - darray_anchor_cdata_get(&parser->anchors);
    ASSERT(ianchor >= 0);
    ASSERT((size_t)ianchor < darray_anchor_size_get(&parser->anchors));
  }

exit:
  out_ianchor->i = (size_t)ianchor;
  return res;
error:
  ianchor = INTPTR_MAX;
  goto exit;
}

static res_T
parse_target
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* target_node,
   struct solparser_target* target,
   struct solparser_pivot_id pivot_id)
{
  enum { POLICY };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && target_node && target);

  if(target_node->type != YAML_MAPPING_NODE) {
    log_err(parser, target_node, "expect a target definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = target_node->data.mapping.pairs.top - target_node->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, target_node->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, target_node->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a target parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the target "Name" is already defined.\n");       \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "anchor")) {
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_ANCHOR;
      res = parse_anchor_alias(parser, val, pivot_id, &target->data.anchor);
    } else if(!strcmp((char*)key->data.scalar.value, "direction")) {
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_DIRECTION;
      res = parse_real3
        (parser, doc, val, -DBL_MAX, DBL_MAX, target->data.direction);
    } else if(!strcmp((char*)key->data.scalar.value, "position")) {
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_POSITION;
      res = parse_real3
        (parser, doc, val, -DBL_MAX, DBL_MAX, target->data.position);
    } else if(!strcmp((char*)key->data.scalar.value, "sun")) {
      /* There is only one sun per YAML file. It is thus sufficient to define
       * the target_type to SOLPARSER_TARGET_SUN to indentify which data is
       * targeted, i.e. it is not necessary to store the identifier of the sun
       * to target */
      struct solparser_sun* sun;
      SETUP_MASK(POLICY, "policy");
      target->type = SOLPARSER_TARGET_SUN;
      res = parse_sun(parser, doc, val, &sun);
    } else {
      log_err(parser, key, "unknown target parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  if(!(mask & BIT(POLICY))) {
    log_err(parser, target_node, "the target policy is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_x_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* x_pivot,
   struct solparser_pivot_id* out_isolpivot)
{
  enum { REF_POINT, TARGET };
  struct solparser_x_pivot* solxpivot = NULL;
  size_t isolpivot = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && x_pivot && out_isolpivot);

  if(x_pivot->type != YAML_MAPPING_NODE) {
    log_err(parser, x_pivot, "expect a x_pivot definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the solstice pivot */
  isolpivot = darray_x_pivot_size_get(&parser->x_pivots);
  res = darray_x_pivot_resize(&parser->x_pivots, isolpivot + 1);
  if(res != RES_OK) {
    log_err(parser, x_pivot, "could not allocate the x_pivot.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  solxpivot = darray_x_pivot_data_get(&parser->x_pivots) + isolpivot;

  n = x_pivot->data.mapping.pairs.top - x_pivot->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, x_pivot->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, x_pivot->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect x_pivot parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the x_pivot parameter `"Name"' is already defined.\n");             \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "ref_point")) {
      SETUP_MASK(REF_POINT, "point");
      res = parse_real3(parser, doc, val, -DBL_MAX, DBL_MAX, solxpivot->ref_point);
    } else if(!strcmp((char*)key->data.scalar.value, "target")) {
      struct solparser_pivot_id pivot_id;
      pivot_id.i = (size_t) (solxpivot - darray_x_pivot_cdata_get(&parser->x_pivots));
      SETUP_MASK(TARGET, "target");
      res = parse_target(parser, doc, val, &solxpivot->target, pivot_id);
    } else {
      log_err(parser, key, "unknown x_pivot parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }
  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, x_pivot, "the x_pivot parameter `"Name"' is missing.\n");\
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(TARGET, "target");
  #undef CHECK_PARAM

  #define DEFAULT_PARAM(Flag, Ptr, Value)                                      \
    if(!(mask & BIT(Flag))) {                                                  \
      *(Ptr) = Value;                                                          \
    } (void)0
  DEFAULT_PARAM(REF_POINT, solxpivot->ref_point, 0);
  DEFAULT_PARAM(REF_POINT, solxpivot->ref_point+1, 0);
  DEFAULT_PARAM(REF_POINT, solxpivot->ref_point+2, 0);
  #undef DEFAULT_PARAM

exit:
  out_isolpivot->i = isolpivot;
  return res;
error:
  if(solxpivot) {
    darray_x_pivot_pop_back(&parser->x_pivots);
    isolpivot = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_zx_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* zx_pivot,
   struct solparser_pivot_id* out_isolpivot)
{
  enum { SPACING, REF_POINT, TARGET };
  struct solparser_zx_pivot* solxzpivot = NULL;
  size_t isolpivot = SIZE_MAX;
  int mask = 0; /* Register the parsed attributes */
  intptr_t i, n;
  res_T res = RES_OK;
  ASSERT(doc && zx_pivot && out_isolpivot);

  if(zx_pivot->type != YAML_MAPPING_NODE) {
    log_err(parser, zx_pivot, "expect a zx_pivot definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the solstice pivot */
  isolpivot = darray_zx_pivot_size_get(&parser->zx_pivots);
  res = darray_zx_pivot_resize(&parser->zx_pivots, isolpivot + 1);
  if(res != RES_OK) {
    log_err(parser, zx_pivot, "could not allocate the zx_pivot.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  solxzpivot = darray_zx_pivot_data_get(&parser->zx_pivots) + isolpivot;

  n = zx_pivot->data.mapping.pairs.top - zx_pivot->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, zx_pivot->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(
      doc, zx_pivot->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect zx_pivot parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the zx_pivot parameter `"Name"' is already defined.\n");            \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*) key->data.scalar.value, "spacing")) {
      SETUP_MASK(SPACING, "spacing");
      res = parse_real(parser, val, 0, DBL_MAX, &solxzpivot->spacing);
    } else if(!strcmp((char*) key->data.scalar.value, "ref_point")) {
      SETUP_MASK(REF_POINT, "ref_point");
      res = parse_real3(
        parser, doc, val, -DBL_MAX, DBL_MAX, solxzpivot->ref_point);
    } else if(!strcmp((char*) key->data.scalar.value, "target")) {
      struct solparser_pivot_id pivot_id;
      pivot_id.i =
        (size_t) (solxzpivot - darray_zx_pivot_cdata_get(&parser->zx_pivots));
      SETUP_MASK(TARGET, "target");
      res = parse_target(parser, doc, val, &solxzpivot->target, pivot_id);
    } else {
      log_err(parser, key, "unknown zx_pivot parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }
  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, zx_pivot,                                                \
         "the zx_pivot parameter `"Name"' is missing.\n");                     \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(TARGET, "target");
  #undef CHECK_PARAM

  #define DEFAULT_PARAM(Flag, Ptr, Value)                                      \
    if(!(mask & BIT(Flag))) {                                                  \
      *(Ptr) = Value;                                                          \
    } (void)0
  DEFAULT_PARAM(SPACING, &solxzpivot->spacing, 0);
  DEFAULT_PARAM(REF_POINT, solxzpivot->ref_point, 0);
  DEFAULT_PARAM(REF_POINT, solxzpivot->ref_point+1, 0);
  DEFAULT_PARAM(REF_POINT, solxzpivot->ref_point+2, 0);
  #undef DEFAULT_PARAM

exit:
  out_isolpivot->i = isolpivot;
  return res;
error:
  if(solxzpivot) {
    darray_zx_pivot_pop_back(&parser->zx_pivots);
    isolpivot = SIZE_MAX;
  }
  goto exit;
}

/*******************************************************************************
 * Sun
 ******************************************************************************/
static res_T
parse_buie
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* buie,
   struct solparser_sun_buie* sun)
{
  enum { CSR };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && buie && sun);

  if(buie->type != YAML_MAPPING_NODE) {
    log_err(parser, buie,
      "expect a buie definition of the sun radial angular distribution.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = buie->data.mapping.pairs.top - buie->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, buie->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, buie->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a buie parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    if(!strcmp((char*)key->data.scalar.value, "csr")) {
      if(mask & BIT(CSR)) {
        log_err(parser, key, "the buie `csr' is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(CSR);
      res = parse_real(parser, val, 1e-6, 0.849, &sun->csr);
    } else {
      log_err(parser, key, "unknown buie parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }

  if(!(mask & BIT(CSR))) {
    log_err(parser, buie, "the buie csr parameter is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_pillbox
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* pillbox,
   struct solparser_sun_pillbox* sun)
{
  enum { APERTURE };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && pillbox && sun);

  if(pillbox->type != YAML_MAPPING_NODE) {
    log_err(parser, pillbox,
      "expect a pillbox definition of the sun radial angular distribution.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = pillbox->data.mapping.pairs.top - pillbox->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, pillbox->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, pillbox->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect a pillbox parameter.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    if(!strcmp((char*)key->data.scalar.value, "aperture")) {
      if(mask & BIT(APERTURE)) {
        log_err(parser, key, "the pillbox `aperture' is already defined.\n");
        res = RES_BAD_ARG;
        goto error;
      }
      mask |= BIT(APERTURE);
      res = parse_real(parser, val, nextafter(0, 1), 90, &sun->aperture);
    } else {
      log_err(parser, pillbox, "unknown pillbox parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
  }

  if(!(mask & BIT(APERTURE))) {
    log_err(parser, pillbox, "the pillbox aperture parameter is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

res_T
parse_sun
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sun,
   struct solparser_sun** out_solsun)
{
  enum { DNI, RADIAL_ANGULAR_DISTRIB, SPECTRUM };
  struct solparser_sun* solsun = NULL;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && sun && out_solsun);

  if(sun == parser->sun_key) {
    solsun = &parser->sun;
    goto exit;
  } else if(parser->sun_key != 0) {
    log_err(parser, sun,
      "a sun is already defined. Previous definition is here %lu:%lu.\n",
      (unsigned long)parser->sun_key->start_mark.line+1,
      (unsigned long)parser->sun_key->start_mark.column+1);
    res = RES_BAD_ARG;
    goto error;
  } else {
    solsun = &parser->sun;
    parser->sun_key = sun;
    solsun->radang_distrib_type = SOLPARSER_SUN_RADANG_DISTRIB_DIRECTIONAL;
  }

  if(sun->type != YAML_MAPPING_NODE) {
    log_err(parser, sun, "expect a sun definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = sun->data.mapping.pairs.top - sun->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, sun->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, sun->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect sun parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key, "the sun "Name" is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "dni")) {
      SETUP_MASK(DNI, "dni");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &solsun->dni);
    } else if(!strcmp((char*)key->data.scalar.value, "buie")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      solsun->radang_distrib_type = SOLPARSER_SUN_RADANG_DISTRIB_BUIE;
      res = parse_buie(parser, doc, val, &solsun->radang_distrib.buie);
    } else if(!strcmp((char*)key->data.scalar.value, "pillbox")) {
      SETUP_MASK(RADIAL_ANGULAR_DISTRIB, "radial angular distribution");
      solsun->radang_distrib_type = SOLPARSER_SUN_RADANG_DISTRIB_PILLBOX;
      res = parse_pillbox(parser, doc, val, &solsun->radang_distrib.pillbox);
    } else if(!strcmp((char*)key->data.scalar.value, "spectrum")) {
      SETUP_MASK(SPECTRUM, "spectrum");
      res = parse_spectrum(parser, doc, 0, DBL_MAX, val, &solsun->spectrum);
    } else {
      log_err(parser, key, "unknown sun parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
      goto error;
    }
    if(res != RES_OK) {
      log_node(parser, key);
      goto error;
    }
    #undef SETUP_MASK
  }

  #define CHECK_PARAM(Flag, Name)                                              \
    if(!(mask & BIT(Flag))) {                                                  \
      log_err(parser, sun, "the sun "Name" is missing.\n");                    \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(DNI, "dni");
  CHECK_PARAM(SPECTRUM, "spectrum");
  #undef CHECK_PARAM

exit:
  *out_solsun = solsun;
  return res;
error:
  if(solsun) {
    solparser_sun_clear(solsun);
    solsun = NULL;
    parser->sun_key = 0;
  }
  goto exit;
}

/*******************************************************************************
 * Item
 ******************************************************************************/
static res_T
parse_item
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* item)
{
  /* Temporary dummy variables */
  struct solparser_material_double_sided_id mtl2;
  struct solparser_entity_id entity;
  struct solparser_geometry_id geometry;
  struct solparser_sun* sun;

  yaml_node_t* key;
  yaml_node_t* val;
  intptr_t n;
  res_T res = RES_OK;
  ASSERT(doc && item);

  if(item->type != YAML_MAPPING_NODE) {
    log_err(parser, item, "expect an item definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = item->data.mapping.pairs.top - item->data.mapping.pairs.start;
  if(n != 1) {
    log_err(parser, item,
      "expect only one \"key:value\" pair while %li are provided.\n", n);
    res = RES_BAD_ARG;
    goto error;
  }

  key = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].key);
  val = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].value);
  if(key->type != YAML_SCALAR_NODE) {
    log_err(parser, key, "expecting an item name.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)key->data.scalar.value, "material")) {
    res = parse_material(parser, doc, val, &mtl2);
  } else if(!strcmp((char*)key->data.scalar.value, "entity")) {
    res = parse_entity(parser, doc, val, &parser->str2entities, &entity);
    if(res == RES_OK) {
      res = flush_deferred_target_aliases(parser, item, entity);
    }
  } else if(!strcmp((char*)key->data.scalar.value, "template")) {
    /* The parsing of the template data is deferred to its explicit used in the
     * definition of an entity. If the parsing of the template becomes a
     * bottleneck, parse the data only once here and cache them for reuse. */
  } else if(!strcmp((char*)key->data.scalar.value, "geometry")) {
    res = parse_geometry(parser, doc, val, &geometry);
  } else if(!strcmp((char*)key->data.scalar.value, "sun")) {
    res = parse_sun(parser, doc, val, &sun);
  } else {
    log_err(parser, key, "unknown item `%s'.\n", key->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }
  if(res != RES_OK) {
    log_node(parser, key);
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solparser_create
  (struct mem_allocator* allocator, struct solparser** out_parser)
{
  struct solparser* parser = NULL;
  struct mem_allocator* mem_allocator;
  res_T res = RES_OK;
  ASSERT(out_parser);

  mem_allocator = allocator ? allocator : &mem_default_allocator;
  parser = MEM_CALLOC(mem_allocator, 1, sizeof(struct solparser));
  if(!parser) {
    fprintf(stderr, "Could not allocate the Solstice parser.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  parser->allocator = mem_allocator;
  ref_init(&parser->ref);
  str_init(mem_allocator, &parser->stream_name);

  /* Materials */
  htable_yaml2sols_init(mem_allocator, &parser->yaml2mtls);
  darray_material_init(mem_allocator, &parser->mtls);
  darray_material2_init(mem_allocator, &parser->mtls2);
  darray_matte_init(mem_allocator, &parser->mattes);
  darray_mirror_init(mem_allocator, &parser->mirrors);
  darray_thin_dielectric_init(mem_allocator, &parser->thin_dielectrics);

  /* Deferred targeted anchors */
  darray_tgtalias_init(mem_allocator, &parser->tgtaliases);

  /* Shapes */
  darray_shape_init(mem_allocator, &parser->shapes);
  darray_cuboid_init(mem_allocator, &parser->cuboids);
  darray_cylinder_init(mem_allocator, &parser->cylinders);
  darray_impgeom_init(mem_allocator, &parser->objs);
  darray_paraboloid_init(mem_allocator, &parser->parabols);
  darray_paraboloid_init(mem_allocator, &parser->parabolic_cylinders);
  darray_hyperboloid_init(mem_allocator, &parser->hyperbols);
  darray_plane_init(mem_allocator, &parser->planes);
  darray_sphere_init(mem_allocator, &parser->spheres);
  darray_impgeom_init(mem_allocator, &parser->stls);

  /* Geometries */
  htable_yaml2sols_init(mem_allocator, &parser->yaml2geoms);
  darray_object_init(mem_allocator, &parser->objects);
  darray_geometry_init(mem_allocator, &parser->geometries);

  /* Sun */
  solparser_sun_init(mem_allocator, &parser->sun);

  /* Entities */
  htable_yaml2sols_init(mem_allocator, &parser->yaml2entities);
  htable_str2sols_init(mem_allocator, &parser->str2entities);
  darray_entity_init(mem_allocator, &parser->entities);

  /* Anchors and pivot(2)s */
  darray_anchor_init(mem_allocator, &parser->anchors);
  darray_x_pivot_init(mem_allocator, &parser->x_pivots);
  darray_zx_pivot_init(mem_allocator, &parser->zx_pivots);

exit:
  *out_parser = parser;
  return res;
error:
  if(parser) {
    solparser_ref_put(parser);
    parser = NULL;
  }
  goto exit;
}

void
solparser_ref_get(struct solparser* parser)
{
  ASSERT(parser);
  ref_get(&parser->ref);
}

void
solparser_ref_put(struct solparser* parser)
{
  ASSERT(parser);
  ref_put(&parser->ref, parser_release);
}

res_T
solparser_setup
  (struct solparser* parser,
   const char* stream_name,
   FILE* stream)
{
  res_T res = RES_OK;
  ASSERT(parser && stream);

  if(parser->parser_is_init) {
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
  }
  res = str_set(&parser->stream_name, stream_name ? stream_name : "<stream>");
  if(res != RES_OK) {
    fprintf(stderr, "Could not register the filename.\n");
    goto error;
  }
  if(!yaml_parser_initialize(&parser->parser)) {
    fprintf(stderr, "Could not initialise the YAML parser.\n");
    res = RES_UNKNOWN_ERR;
    goto error;
  }
  parser->parser_is_init = 1;
  yaml_parser_set_input_file(&parser->parser, stream);

exit:
  return res;
error:
  str_clear(&parser->stream_name);
  if(parser->parser_is_init) {
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
  }
  goto exit;
}

res_T
solparser_load(struct solparser* parser)
{
  yaml_document_t doc;
  yaml_node_t* root;
  const char* filename;
  intptr_t i, n;
  int doc_is_init = 0;
  res_T res = RES_OK;
  ASSERT(parser);

  filename = str_cget(&parser->stream_name);

  parser_clear(parser); /* Clean up previously loaded data */

  if(!parser->parser_is_init) {
    res = RES_BAD_OP;
    goto error;
  }

  if(!yaml_parser_load(&parser->parser, &doc)) {
    fprintf(stderr, "%s:%lu:%lu: %s.\n",
      filename,
      (unsigned long)parser->parser.problem_mark.line+1,
      (unsigned long)parser->parser.problem_mark.column+1,
      parser->parser.problem);
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }
  doc_is_init = 1;

  root = yaml_document_get_root_node(&doc);
  if(!root) {
    yaml_parser_delete(&parser->parser);
    parser->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }

  if(root->type != YAML_SEQUENCE_NODE) {
    log_err(parser, root, "expect a list of items.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = root->data.sequence.items.top - root->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* item;

    item = yaml_document_get_node(&doc, root->data.sequence.items.start[i]);
    res = parse_item(parser, &doc, item);
    if(res != RES_OK) goto error;
  }

  if(!parser->sun_key) {
    log_err(parser, root, "no sun definition in the document.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  if(doc_is_init) yaml_document_delete(&doc);
  return res;
error:
  parser_clear(parser);
  goto exit;
}

const struct solparser_anchor*
solparser_find_anchor
  (struct solparser* parser, const char* name)
{
  struct str str;
  struct str str_tk;
  struct htable_str2sols* htable = NULL;
  struct solparser_entity* entity = NULL;
  struct solparser_anchor* anchor = NULL;
  char* cstr;
  char* tk;
  char* tk_anchor;
  res_T res = RES_OK;
  ASSERT(parser && name);

  str_init(parser->allocator, &str);
  str_init(parser->allocator, &str_tk);

  res = str_set(&str, name);
  if(res != RES_OK) {
    fprintf(stderr, "%s: could not copy the input string.\n", FUNC_NAME);
    goto error;
  }
  res = str_reserve(&str_tk, str_len(&str));
  if(res != RES_OK) {
    fprintf(stderr, "%s: could not allocate the temporary token string.\n",
      FUNC_NAME);
    goto error;
  }

  cstr = str_get(&str);
  tk_anchor = strrchr(cstr, '.');
  if(!tk_anchor) goto exit;
  *tk_anchor='\0';
  ++tk_anchor;

  tk = strtok(cstr, ".");
  htable = &parser->str2entities;
  while(tk) {
    size_t* pientity;
    str_set(&str_tk, tk);
    pientity = htable_str2sols_find(htable, &str_tk);
    if(!pientity) {
      tk = NULL;
      entity = NULL;
    } else {
      tk = strtok(NULL, ".");
      entity = darray_entity_data_get(&parser->entities) + *pientity;
      htable = &entity->str2children;
    }
  }

  if(entity) {
    size_t* pianchor;
    str_set(&str_tk, tk_anchor);
    pianchor = htable_str2sols_find(&entity->str2anchors, &str_tk);
    if(pianchor) {
      anchor = darray_anchor_data_get(&parser->anchors) + *pianchor;
    }
  }

exit:
  str_release(&str);
  str_release(&str_tk);
  return anchor;
error:
  anchor = NULL;
  goto exit;
}

const struct solparser_entity*
solparser_find_entity
  (struct solparser* parser, const char* name)
{
  struct htable_str2sols* htable = NULL;
  struct solparser_entity* entity = NULL;
  struct str str;
  struct str str_tk;
  char* cstr;
  char* tk;
  res_T res = RES_OK;
  ASSERT(parser && name);

  str_init(parser->allocator, &str);
  str_init(parser->allocator, &str_tk);

  res = str_set(&str, name);
  if(res != RES_OK) {
    fprintf(stderr, "%s: could not copy the input string.\n", FUNC_NAME);
    goto error;
  }
  res = str_reserve(&str_tk, str_len(&str));
  if(res != RES_OK) {
    fprintf(stderr, "%s: could not allocate the temporary token sting.\n",
      FUNC_NAME);
    goto error;
  }

  cstr = str_get(&str);
  tk = strtok(cstr, ".");
  htable = &parser->str2entities;
  while(tk) {
    size_t* pientity;
    str_set(&str_tk, tk);
    pientity = htable_str2sols_find(htable, &str_tk);
    if(!pientity) {
      tk = NULL;
      entity = NULL;
    } else {
      tk = strtok(NULL, ".");
      entity = darray_entity_data_get(&parser->entities) + *pientity;
      htable = &entity->str2children;
    }
  }

exit:
  str_release(&str);
  str_release(&str_tk);
  return entity;
error:
  entity = NULL;
  goto exit;
}

const struct solparser_anchor*
solparser_get_anchor
  (const struct solparser* parser,
   const struct solparser_anchor_id anchor)
{
  ASSERT(parser && anchor.i < darray_anchor_size_get(&parser->anchors));
  return darray_anchor_cdata_get(&parser->anchors) + anchor.i;
}

const struct solparser_entity*
solparser_get_entity
  (const struct solparser* parser,
   const struct solparser_entity_id entity)
{
  ASSERT(parser && entity.i < darray_entity_size_get(&parser->entities));
  return darray_entity_cdata_get(&parser->entities) + entity.i;
}

const struct solparser_geometry*
solparser_get_geometry
  (const struct solparser* parser,
   const struct solparser_geometry_id geom)
{
  ASSERT(parser && geom.i < darray_geometry_size_get(&parser->geometries));
  return darray_geometry_cdata_get(&parser->geometries) + geom.i;
}

const struct solparser_material*
solparser_get_material
  (const struct solparser* parser,
   const struct solparser_material_id mtl)
{
  ASSERT(parser && mtl.i < darray_material_size_get(&parser->mtls));
  return darray_material_cdata_get(&parser->mtls) + mtl.i;
}

const struct solparser_material_double_sided*
solparser_get_material_double_sided
  (const struct solparser* parser,
   const struct solparser_material_double_sided_id mtl2)
{
  ASSERT(parser && mtl2.i < darray_material2_size_get(&parser->mtls2));
  return darray_material2_cdata_get(&parser->mtls2) + mtl2.i;
}

const struct solparser_material_matte*
solparser_get_material_matte
  (const struct solparser* parser,
   const struct solparser_material_matte_id matte)
{
  ASSERT(parser && matte.i < darray_matte_size_get(&parser->mattes));
  return darray_matte_cdata_get(&parser->mattes) + matte.i;
}

const struct solparser_material_mirror*
solparser_get_material_mirror
  (const struct solparser* parser,
   const struct solparser_material_mirror_id mirror)
{
  ASSERT(parser && mirror.i < darray_mirror_size_get(&parser->mirrors));
  return darray_mirror_cdata_get(&parser->mirrors) + mirror.i;
}

const struct solparser_material_thin_dielectric*
solparser_get_material_thin_dielectric
  (const struct solparser* parser,
   const struct solparser_material_thin_dielectric_id thin)
{
  ASSERT(parser);
  ASSERT(thin.i < darray_thin_dielectric_size_get(&parser->thin_dielectrics));
  return darray_thin_dielectric_cdata_get(&parser->thin_dielectrics) + thin.i;
}

const struct solparser_object*
solparser_get_object
  (const struct solparser* parser,
   const struct solparser_object_id obj)
{
  ASSERT(parser && obj.i < darray_object_size_get(&parser->objects));
  return darray_object_cdata_get(&parser->objects) + obj.i;
}

const struct solparser_x_pivot*
solparser_get_x_pivot
  (const struct solparser* parser,
   const struct solparser_pivot_id x_pivot)
{
  ASSERT(parser && x_pivot.i < darray_x_pivot_size_get(&parser->x_pivots));
  return darray_x_pivot_cdata_get(&parser->x_pivots) + x_pivot.i;
}

const struct solparser_zx_pivot*
solparser_get_zx_pivot
  (const struct solparser* parser,
   const struct solparser_pivot_id zx_pivot)
{
  ASSERT(parser && zx_pivot.i < darray_zx_pivot_size_get(&parser->zx_pivots));
  return darray_zx_pivot_cdata_get(&parser->zx_pivots) + zx_pivot.i;
}

const struct solparser_shape*
solparser_get_shape
  (const struct solparser* parser,
   const struct solparser_shape_id shape)
{
  ASSERT(parser && shape.i < darray_shape_size_get(&parser->shapes));
  return darray_shape_cdata_get(&parser->shapes) + shape.i;
}

const struct solparser_shape_cuboid*
solparser_get_shape_cuboid
  (const struct solparser* parser,
   const struct solparser_shape_cuboid_id cuboid)
{
  ASSERT(parser && cuboid.i < darray_cuboid_size_get(&parser->cuboids));
  return darray_cuboid_cdata_get(&parser->cuboids) + cuboid.i;
}

const struct solparser_shape_cylinder*
solparser_get_shape_cylinder
  (const struct solparser* parser,
   const struct solparser_shape_cylinder_id cylinder)
{
  ASSERT(parser && cylinder.i < darray_cylinder_size_get(&parser->cylinders));
  return darray_cylinder_cdata_get(&parser->cylinders) + cylinder.i;
}

const struct solparser_shape_imported_geometry*
solparser_get_shape_obj
  (const struct solparser* parser,
   const struct solparser_shape_imported_geometry_id impgeom)
{
  ASSERT(parser && impgeom.i < darray_impgeom_size_get(&parser->objs));
  return darray_impgeom_cdata_get(&parser->objs) + impgeom.i;
}

const struct solparser_shape_paraboloid*
solparser_get_shape_parabol
  (const struct solparser* parser,
   const struct solparser_shape_paraboloid_id paraboloid)
{
  ASSERT(parser && paraboloid.i < darray_paraboloid_size_get(&parser->parabols));
  return darray_paraboloid_cdata_get(&parser->parabols) + paraboloid.i;
}

const struct solparser_shape_paraboloid*
solparser_get_shape_parabolic_cylinder
  (const struct solparser* parser,
   const struct solparser_shape_paraboloid_id paraboloid)
{
  ASSERT(parser);
  ASSERT(paraboloid.i<darray_paraboloid_size_get(&parser->parabolic_cylinders));
  return darray_paraboloid_cdata_get(&parser->parabolic_cylinders)+paraboloid.i;
}

const struct solparser_shape_hyperboloid*
solparser_get_shape_hyperbol
  (const struct solparser* parser,
   const struct solparser_shape_hyperboloid_id hyperboloid)
{
  ASSERT(parser && hyperboloid.i < darray_hyperboloid_size_get(&parser->hyperbols));
  return darray_hyperboloid_cdata_get(&parser->hyperbols) + hyperboloid.i;
}

const struct solparser_shape_plane*
solparser_get_shape_plane
  (const struct solparser* parser,
   const struct solparser_shape_plane_id plane)
{
  ASSERT(parser && plane.i < darray_plane_size_get(&parser->planes));
  return darray_plane_cdata_get(&parser->planes) + plane.i;
}

const struct solparser_shape_sphere*
solparser_get_shape_sphere
  (const struct solparser* parser,
   const struct solparser_shape_sphere_id sphere)
{
  ASSERT(parser && sphere.i < darray_sphere_size_get(&parser->spheres));
  return darray_sphere_cdata_get(&parser->spheres) + sphere.i;
}

const struct solparser_shape_imported_geometry*
solparser_get_shape_stl
  (const struct solparser* parser,
   const struct solparser_shape_imported_geometry_id impgeom)
{
  ASSERT(parser && impgeom.i < darray_impgeom_size_get(&parser->stls));
  return darray_impgeom_cdata_get(&parser->stls) + impgeom.i;
}

const struct solparser_sun*
solparser_get_sun(const struct solparser* parser)
{
  ASSERT(parser && parser->sun_key);
  return &parser->sun;
}

void
solparser_entity_iterator_begin
  (struct solparser* parser,
   struct solparser_entity_iterator* it)
{
  ASSERT(parser && it);
  htable_str2sols_begin(&parser->str2entities, &it->it__);
}

void
solparser_entity_iterator_end
  (struct solparser* parser,
   struct solparser_entity_iterator* it)
{
  ASSERT(parser && it);
  htable_str2sols_end(&parser->str2entities, &it->it__);
}

void
solparser_material_iterator_begin
  (struct solparser* parser, struct solparser_material_iterator* it)
{
  ASSERT(parser && it);
  it->mtls__ = darray_material_cdata_get(&parser->mtls);
  it->imtl__ = 0;
}

void
solparser_material_iterator_end
  (struct solparser* parser, struct solparser_material_iterator* it)
{
  ASSERT(parser && it);
  it->mtls__ = darray_material_cdata_get(&parser->mtls);
  it->imtl__ = darray_material_size_get(&parser->mtls);
}

void
solparser_geometry_iterator_begin
  (struct solparser* parser, struct solparser_geometry_iterator* it)
{
  ASSERT(parser && it);
  it->geoms__ = darray_geometry_cdata_get(&parser->geometries);
  it->igeom__ = 0;
}

void
solparser_geometry_iterator_end
  (struct solparser* parser, struct solparser_geometry_iterator* it)
{
  ASSERT(parser && it);
  it->geoms__ = darray_geometry_cdata_get(&parser->geometries);
  it->igeom__ = darray_geometry_size_get(&parser->geometries);
}

