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

#define _POSIX_C_SOURCE 200112L /* nextafter support */

#include "solparser_c.h"

#include <rsys/cstr.h>

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
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

static res_T
parse_item
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* item)
{
  /* Temporary dummy variables */
  struct solparser_entity_id entity;
  struct solparser_geometry_id geometry;
  struct solparser_material_double_sided_id mtl2;
  struct solparser_medium_id medium;
  struct solparser_sun* sun;
  struct solparser_atmosphere* atmosphere;

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

  /* The parsing of the templates/spectra is deferred to their explicit use */
  if(!strcmp((char*)key->data.scalar.value, "material")) {
    res = parse_material(parser, doc, val, &mtl2);
  } else if(!strcmp((char*)key->data.scalar.value, "medium")) {
    res = parse_medium(parser, doc, val, &medium);
  } else if(!strcmp((char*)key->data.scalar.value, "entity")) {
    res = parse_entity(parser, doc, val, &parser->str2entities, &entity);
    if(res == RES_OK) {
      res = flush_deferred_target_aliases(parser, item, entity);
    }
  } else if(!strcmp((char*)key->data.scalar.value, "template")) { /* Deferred */
  } else if(!strcmp((char*)key->data.scalar.value, "geometry")) {
    res = parse_geometry(parser, doc, val, &geometry);
  } else if(!strcmp((char*)key->data.scalar.value, "sun")) {
    res = parse_sun(parser, doc, val, &sun);
  } else if (!strcmp((char*)key->data.scalar.value, "atmosphere")) {
    res = parse_atmosphere(parser, doc, val, &atmosphere);
  } else if(!strcmp((char*)key->data.scalar.value, "spectrum")) { /* Deferred */
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

/* Clean up loaded data */
static INLINE void
parser_clear(struct solparser* parser)
{
  ASSERT(parser);

  /* Materials */
  htable_yaml2sols_clear(&parser->yaml2mtls);
  darray_image_clear(&parser->images);
  darray_material_clear(&parser->mtls);
  darray_material2_clear(&parser->mtls2);
  darray_dielectric_clear(&parser->dielectrics);
  darray_matte_clear(&parser->mattes);
  darray_mirror_clear(&parser->mirrors);
  darray_thin_dielectric_clear(&parser->thin_dielectrics);

  /* Mediums */
  htable_yaml2sols_clear(&parser->yaml2mediums);
  darray_medium_clear(&parser->mediums);

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
  darray_hemisphere_clear(&parser->hemispheres);
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

  /* Atmosphere */
  solparser_atmosphere_clear(&parser->atmosphere);
  parser->atmosphere_key = 0;

  /* Entities */
  htable_str2sols_clear(&parser->str2entities);
  darray_entity_clear(&parser->entities);

  /* Miscellaneous */
  darray_anchor_clear(&parser->anchors);
  darray_x_pivot_clear(&parser->x_pivots);
  darray_zx_pivot_clear(&parser->zx_pivots);
  darray_spectrum_clear(&parser->spectra);
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
  darray_image_release(&parser->images);
  darray_material_release(&parser->mtls);
  darray_material2_release(&parser->mtls2);
  darray_dielectric_release(&parser->dielectrics);
  darray_matte_release(&parser->mattes);
  darray_mirror_release(&parser->mirrors);
  darray_thin_dielectric_release(&parser->thin_dielectrics);

  /* Mediums */
  htable_yaml2sols_release(&parser->yaml2mediums);
  darray_medium_release(&parser->mediums);

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
  darray_hemisphere_release(&parser->hemispheres);
  darray_plane_release(&parser->planes);
  darray_sphere_release(&parser->spheres);
  darray_impgeom_release(&parser->stls);

  /* Geometries */
  htable_yaml2sols_release(&parser->yaml2geoms);
  darray_object_release(&parser->objects);
  darray_geometry_release(&parser->geometries);

  /* Sun */
  solparser_sun_release(&parser->sun);

  /* Atmosphere */
  solparser_atmosphere_release(&parser->atmosphere);

  /* Entities */
  htable_str2sols_release(&parser->str2entities);
  darray_entity_release(&parser->entities);

  /* Miscellaneous */
  darray_anchor_release(&parser->anchors);
  darray_x_pivot_release(&parser->x_pivots);
  darray_zx_pivot_release(&parser->zx_pivots);
  darray_spectrum_release(&parser->spectra);

  MEM_RM(parser->allocator, parser);
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
void
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

void
log_node(const struct solparser* parser, const yaml_node_t* node)
{
  fprintf(stderr, "\tby %s:%lu:%lu\n",
    str_cget(&parser->stream_name),
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
}

res_T
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

res_T
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
    log_err(parser, realX, "expect a sequence of %lu reals.\n", 
      (unsigned long)dim);
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

res_T
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

res_T
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

res_T
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

/*******************************************************************************
 * Exported functions
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
  darray_image_init(mem_allocator, &parser->images);
  darray_material_init(mem_allocator, &parser->mtls);
  darray_material2_init(mem_allocator, &parser->mtls2);
  darray_dielectric_init(mem_allocator, &parser->dielectrics);
  darray_matte_init(mem_allocator, &parser->mattes);
  darray_mirror_init(mem_allocator, &parser->mirrors);
  darray_thin_dielectric_init(mem_allocator, &parser->thin_dielectrics);

  /* Mediums */
  htable_yaml2sols_init(mem_allocator, &parser->yaml2mediums);
  darray_medium_init(mem_allocator, &parser->mediums);

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
  darray_hemisphere_init(mem_allocator, &parser->hemispheres);
  darray_plane_init(mem_allocator, &parser->planes);
  darray_sphere_init(mem_allocator, &parser->spheres);
  darray_impgeom_init(mem_allocator, &parser->stls);

  /* Geometries */
  htable_yaml2sols_init(mem_allocator, &parser->yaml2geoms);
  darray_object_init(mem_allocator, &parser->objects);
  darray_geometry_init(mem_allocator, &parser->geometries);

  /* Sun */
  solparser_sun_init(mem_allocator, &parser->sun);

  /* Atmosphere */
  solparser_atmosphere_init(mem_allocator, &parser->atmosphere);

  /* Entities */
  htable_str2sols_init(mem_allocator, &parser->str2entities);
  darray_entity_init(mem_allocator, &parser->entities);

  /* Miscellaneous */
  darray_anchor_init(mem_allocator, &parser->anchors);
  darray_x_pivot_init(mem_allocator, &parser->x_pivots);
  darray_zx_pivot_init(mem_allocator, &parser->zx_pivots);
  darray_spectrum_init(mem_allocator, &parser->spectra);

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

const struct solparser_image*
solparser_get_image
  (const struct solparser* parser,
   const struct solparser_image_id image)
{
  ASSERT(parser && image.i < darray_image_size_get(&parser->images));
  return darray_image_cdata_get(&parser->images) + image.i;
}

const struct solparser_geometry*
solparser_get_geometry
  (const struct solparser* parser,
   const struct solparser_geometry_id geom)
{
  ASSERT(parser && geom.i < darray_geometry_size_get(&parser->geometries));
  return darray_geometry_cdata_get(&parser->geometries) + geom.i;
}

const struct solparser_medium*
solparser_get_medium
  (const struct solparser* parser,
   const struct solparser_medium_id medium)
{
  ASSERT(parser && medium.i < darray_medium_size_get(&parser->mediums));
  return darray_medium_cdata_get(&parser->mediums) + medium.i;
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

const struct solparser_material_dielectric*
solparser_get_material_dielectric
  (const struct solparser* parser,
   const struct solparser_material_dielectric_id dielectric)
{
  ASSERT(parser);
  ASSERT(dielectric.i < darray_dielectric_size_get(&parser->dielectrics));
  return darray_dielectric_cdata_get(&parser->dielectrics) + dielectric.i;
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

const struct solparser_shape_hemisphere*
solparser_get_shape_hemisphere
  (const struct solparser* parser,
   const struct solparser_shape_hemisphere_id hemisphere)
{
  ASSERT(parser && hemisphere.i < darray_hemisphere_size_get(&parser->hemispheres));
  return darray_hemisphere_cdata_get(&parser->hemispheres) + hemisphere.i;
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

const struct solparser_spectrum*
solparser_get_spectrum
  (const struct solparser* parser,
   const struct solparser_spectrum_id spectrum)
{
  ASSERT(parser && spectrum.i < darray_spectrum_size_get(&parser->spectra));
  return darray_spectrum_cdata_get(&parser->spectra) + spectrum.i;
}

int
solparser_has_spectrum(const struct solparser* parser)
{
  ASSERT(parser);
  return darray_spectrum_size_get(&parser->spectra) != 0;
}

const struct solparser_sun*
solparser_get_sun(const struct solparser* parser)
{
  ASSERT(parser && parser->sun_key);
  return &parser->sun;
}

const struct solparser_atmosphere*
solparser_get_atmosphere(const struct solparser* parser)
{
  ASSERT(parser);
  if(parser->atmosphere_key) return &parser->atmosphere;
  else return NULL;
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

