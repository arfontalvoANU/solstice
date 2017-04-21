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

#include "solparser_c.h"
#include <math.h> /* nextafter */

/*******************************************************************************
 * Helper functions
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
  enum { HEIGHT, RADIUS, SLICES, STACKS };
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
  shape->nslices = 16; /* default value */
  shape->nstacks = 1; /* default value */
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
    }
    else if(!strcmp((char*)key->data.scalar.value, "stacks")) {
      SETUP_MASK(STACKS, "stacks");
      res = parse_integer(parser, val, 1, 4096, &shape->nstacks);
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
  enum { CLIP, FOCAL, SLICES };
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
    } else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 4, 4096, &shape->nslices);
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
parse_hyperboloid
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* hyperboloid,
   struct solparser_shape_hyperboloid_id* out_ishape)
{
  enum { CLIP, FOCAL, SLICES };
  struct solparser_shape_hyperboloid* shape = NULL;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && hyperboloid && out_ishape);

  if(hyperboloid->type != YAML_MAPPING_NODE) {
    log_err(parser, hyperboloid, "expect a mapping of hyperbol parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a hyperboloid shape */
  ishape = darray_hyperboloid_size_get(&parser->hyperbols);
  res = darray_hyperboloid_resize(&parser->hyperbols, ishape + 1);
  if(res != RES_OK) {
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
    if(key->type != YAML_SCALAR_NODE) {
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
    if(!strcmp((char*) key->data.scalar.value, "clip")) {
      SETUP_MASK(CLIP, "clip");
      res = parse_clip(parser, doc, val, &shape->polyclips);
    } else if(!strcmp((char*)key->data.scalar.value, "focals")) {
      SETUP_MASK(FOCAL, "focals");
      res = parse_focals_description(parser, doc, val, &shape->focals);
    } else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 4, 4096, &shape->nslices);
    } else {
      log_err(parser, key, "unknown hyperbol parameter `%s'.\n",
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
  if(shape) {
    darray_hyperboloid_pop_back(&parser->hyperbols);
    ishape = SIZE_MAX;
  }
  goto exit;
}

static res_T
parse_hemisphere
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* hemisphere,
   struct solparser_shape_hemisphere_id* out_ishape)
{
  enum { CLIP, RADIUS, SLICES };
  struct solparser_shape_hemisphere* shape = NULL;
  size_t ishape = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && hemisphere && out_ishape);

  if(hemisphere->type != YAML_MAPPING_NODE) {
    log_err(parser, hemisphere, "expect a mapping of hemisphere parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate a hemispheric shape */
  ishape = darray_hemisphere_size_get(&parser->hemispheres);
  res = darray_hemisphere_resize(&parser->hemispheres, ishape + 1);
  if(res != RES_OK) {
    log_err(parser, hemisphere, "could not allocate the hemisphere shape.\n");
    goto error;
  }
  shape = darray_hemisphere_data_get(&parser->hemispheres) + ishape;

  n = hemisphere->data.mapping.pairs.top - hemisphere->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, hemisphere->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, hemisphere->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect hemisphere parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the hemisphere parameter `"Name"' is already defined.\n");          \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "clip")) {
      SETUP_MASK(CLIP, "clip");
      res = parse_clip(parser, doc, val, &shape->polyclips);
    }
    else if(!strcmp((char*)key->data.scalar.value, "radius")) {
      SETUP_MASK(RADIUS, "radius");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &shape->radius);
    }
    else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 4, 4096, &shape->nslices);
    }
    else {
      log_err(parser, key, "unknown hemisphere parameter `%s'.\n",
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
      log_err(parser, hemisphere,                                              \
        "the hemisphere parameter `"Name"' is missing.\n");                    \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(RADIUS, "radius");
  #undef CHECK_PARAM

exit :
  out_ishape->i = ishape;
  return res;
error:
  if(shape) {
    darray_hemisphere_pop_back(&parser->hemispheres);
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
  enum { CLIP, SLICES };
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
  shape->nslices = 1; /* default value */
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
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the plane parameter `"Name"' is already defined.\n");               \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0

    if(!strcmp((char*)key->data.scalar.value, "clip")) {
      SETUP_MASK(CLIP, "clip");
      res = parse_clip(parser, doc, val, &shape->polyclips);
    } else if(!strcmp((char*)key->data.scalar.value, "slices")) {
      SETUP_MASK(SLICES, "slices");
      res = parse_integer(parser, val, 1, 4096, &shape->nslices);
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
    #undef SETUP_MASK
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
  enum { RADIUS, SLICES, STACKS };
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
  shape->nslices = 16; /* default value */
  shape->nstacks = 8; /* initial default value */
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
      if(!(mask & BIT(STACKS)))
        shape->nstacks = shape->nslices / 2; /* if unset, new default value */
    } else if(!strcmp((char*)key->data.scalar.value, "stacks")) {
      SETUP_MASK(STACKS, "stacks");
      res = parse_integer(parser, val, 2, 4096, &shape->nstacks);
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
    } else if(!strcmp((char*) key->data.scalar.value, "hyperbol")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_HYPERBOL;
      res = parse_hyperboloid(parser, doc, val, &shape->data.hyperbol);
    }
    else if(!strcmp((char*)key->data.scalar.value, "hemisphere")) {
      SETUP_MASK(SHAPE, "shape");
      shape->type = SOLPARSER_SHAPE_HEMISPHERE;
      res = parse_hemisphere(parser, doc, val, &shape->data.hemisphere);
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

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
parse_focals_description
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* desc,
   struct solparser_hyperboloid_focals* focals)
{
  enum { REAL, IMAGE };
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(doc && desc && focals);

  if(desc->type != YAML_MAPPING_NODE) {
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
    if(key->type != YAML_SCALAR_NODE) {
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
    if(!strcmp((char*) key->data.scalar.value, "real")) {
      SETUP_MASK(REAL, "real");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &focals->real);
    } else if(!strcmp((char*) key->data.scalar.value, "image")) {
      SETUP_MASK(IMAGE, "image");
      res = parse_real(parser, val, nextafter(0, 1), DBL_MAX, &focals->image);
    }
    if(res != RES_OK) {
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

res_T
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



