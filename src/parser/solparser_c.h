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

#ifndef SOLPARSER_C_H
#define SOLPARSER_C_H

#include "solparser.h"
#include "solparser_atmosphere.h"
#include "solparser_entity.h"
#include "solparser_image.h"
#include "solparser_material.h"
#include "solparser_medium.h"
#include "solparser_mtl_data.h"
#include "solparser_pivot.h"
#include "solparser_shape.h"
#include "solparser_spectrum.h"
#include "solparser_sun.h"

#include <rsys/dynamic_array.h>
#include <rsys/hash_table.h>
#include <rsys/ref_count.h>
#include <rsys/str.h>

#include <yaml.h>

struct target_alias {
  struct solparser_pivot_id pivot;
  const yaml_node_t* alias; /* Anchor */
};

/* Declare the target_alias array */
#define DARRAY_NAME tgtalias
#define DARRAY_DATA struct target_alias
#include <rsys/dynamic_array.h>

/* Declare the array of mediums */
#define DARRAY_NAME medium
#define DARRAY_DATA struct solparser_medium
#include <rsys/dynamic_array.h>

/* Declare the array of dielectric materials */
#define DARRAY_NAME dielectric
#define DARRAY_DATA struct solparser_material_dielectric
#define DARRAY_FUNCTOR_INIT solparser_material_dielectric_init
#include <rsys/dynamic_array.h>

/* Declare the array of matte materials */
#define DARRAY_NAME matte
#define DARRAY_DATA struct solparser_material_matte
#define DARRAY_FUNCTOR_INIT solparser_material_matte_init
#include <rsys/dynamic_array.h>

/* Declare the array of mirror materials */
#define DARRAY_NAME mirror
#define DARRAY_DATA struct solparser_material_mirror
#define DARRAY_FUNCTOR_INIT solparser_material_mirror_init
#include <rsys/dynamic_array.h>

/* Declare the array of thin_dielectric materials */
#define DARRAY_NAME thin_dielectric
#define DARRAY_DATA struct solparser_material_thin_dielectric
#define DARRAY_FUNCTOR_INIT solparser_material_thin_dielectric_init
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

/* Declare the array of images */
#define DARRAY_NAME image
#define DARRAY_DATA struct solparser_image
#define DARRAY_FUNCTOR_INIT solparser_image_init
#define DARRAY_FUNCTOR_RELEASE solparser_image_release
#define DARRAY_FUNCTOR_COPY solparser_image_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solparser_image_copy_and_release
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

/* Declare the array of hemispheres */
#define DARRAY_NAME hemisphere
#define DARRAY_DATA struct solparser_shape_hemisphere
#define DARRAY_FUNCTOR_INIT solparser_shape_hemisphere_init
#define DARRAY_FUNCTOR_RELEASE solparser_shape_hemisphere_release
#define DARRAY_FUNCTOR_COPY solparser_shape_hemisphere_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE \
  solparser_shape_hemisphere_copy_and_release
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

/* Declare the array of spectra */
#define DARRAY_NAME spectrum
#define DARRAY_DATA struct solparser_spectrum
#define DARRAY_FUNCTOR_INIT solparser_spectrum_init
#define DARRAY_FUNCTOR_RELEASE solparser_spectrum_release
#define DARRAY_FUNCTOR_COPY solparser_spectrum_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE solparser_spectrum_copy_and_release
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

  /* Material */
  struct htable_yaml2sols yaml2mtls; /* Cache of materials */
  struct darray_image images;
  struct darray_material mtls;
  struct darray_material2 mtls2; /* Double sided materials */
  struct darray_dielectric dielectrics;
  struct darray_matte mattes;
  struct darray_mirror mirrors;
  struct darray_thin_dielectric thin_dielectrics;

  /* Medium */
  struct htable_yaml2sols yaml2mediums; /* Cache of mediums */
  struct darray_medium mediums;

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
  struct darray_hemisphere hemispheres;
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

  /* Atmosphere. Note that at most one atmosphere is supported */
  const yaml_node_t* atmosphere_key; /* ptr of the atmosphere. Can be NULL */
  struct solparser_atmosphere atmosphere; /* The loaded atmosphere, if any */

  /* Entity */
  struct htable_str2sols str2entities;
  struct darray_entity entities;

  /* Miscellaneous */
  struct darray_anchor anchors;
  struct darray_x_pivot x_pivots;
  struct darray_zx_pivot zx_pivots;
  struct darray_spectrum spectra;

  ref_T ref;
  struct mem_allocator* allocator;
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
extern LOCAL_SYM void
log_err
  (const struct solparser* parser,
   const yaml_node_t* node,
   const char* fmt,
  ...)
#ifdef COMPILER_GCC
  __attribute((format(printf, 3, 4)))
#endif
  ;

extern LOCAL_SYM void
log_node
  (const struct solparser* parser,
   const yaml_node_t* node);


/*******************************************************************************
 * Miscellaneous parsing functions
 ******************************************************************************/
extern LOCAL_SYM res_T
parse_real
  (struct solparser* parser,
   const yaml_node_t* real,
   const double lower_bound,
   const double upper_bound,
   double* dst);

extern LOCAL_SYM res_T
parse_realX
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* realX,
   const double lower_bound,
   const double upper_bound,
   const size_t dim,
   double dst[]);

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

extern LOCAL_SYM res_T
parse_integer
  (struct solparser* parser,
   yaml_node_t* integer,
   const long lower_bound,
   const long upper_bound,
   long* dst);

extern LOCAL_SYM res_T
parse_string
  (struct solparser* parser,
   yaml_node_t* string,
   struct str* str);

extern LOCAL_SYM res_T
parse_transform
   (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* transform,
   double translation[3],
   double rotation[3]);

/*******************************************************************************
 * Main parsing functions
 ******************************************************************************/
extern LOCAL_SYM res_T
parse_entity
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* entity,
   struct htable_str2sols* htable,
   struct solparser_entity_id* out_isolent);

extern LOCAL_SYM res_T
parse_image
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* image,
   struct solparser_image_id* out_img);

extern LOCAL_SYM res_T
parse_focals_description
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* desc,
   struct solparser_hyperboloid_focals* focals);

extern LOCAL_SYM res_T
parse_geometry
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* geometry,
   struct solparser_geometry_id* out_isolgeom);

extern LOCAL_SYM res_T
parse_material
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* mtl,
   struct solparser_material_double_sided_id* out_imtl2);

extern LOCAL_SYM res_T
parse_medium
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* medium,
   struct solparser_medium_id* out_imedium);

extern LOCAL_SYM res_T
parse_mtl_data
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* mtl_data,
   const double lower_bound,
   const double upper_bound,
   struct solparser_mtl_data* data);

extern LOCAL_SYM res_T
parse_spectrum
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* spectrum,
   const double lower_bound,
   const double upper_bound,
   struct solparser_spectrum_id* out_ispectrum);

extern LOCAL_SYM res_T
parse_sun
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* sun,
   struct solparser_sun** out_solsun);

extern LOCAL_SYM res_T
parse_atmosphere
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* atm,
   struct solparser_atmosphere** out_solatm);

extern LOCAL_SYM res_T
parse_x_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* x_pivot,
   struct solparser_pivot_id* out_isolpivot);

extern LOCAL_SYM res_T
parse_zx_pivot
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* zx_pivot,
   struct solparser_pivot_id* out_isolpivot);

#endif /* SOLPARSER_C_H */
