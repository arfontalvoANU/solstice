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

#include "solparser_c.h"

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
parse_image
  (struct solparser* parser,
   yaml_document_t* doc,
   const yaml_node_t* img,
   struct solparser_image_id* out_iimg)
{
  enum { PATH };
  struct solparser_image* solimg = NULL;
  size_t isolimg = SIZE_MAX;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(parser && doc && img &&out_iimg);

  if(img->type != YAML_MAPPING_NODE) {
    log_err(parser, img, "expect a mapping of image parameters.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate an image */
  isolimg = darray_image_size_get(&parser->images);
  res = darray_image_resize(&parser->images, isolimg + 1);
  if(res != RES_OK) {
    log_err(parser, img, "could not allocate the image.\n");
    goto error;
  }
  solimg = darray_image_data_get(&parser->images) + isolimg;

  n = img->data.mapping.pairs.top - img->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, img->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, img->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(parser, key, "expect image parameters.\n");
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(parser, key,                                                   \
          "the image parameter `"Name"' is already defined.\n");               \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "path")) {
      SETUP_MASK(PATH, "path");
      res = parse_string(parser, val, &solimg->filename);
    } else {
      log_err(parser, key, "unknown image parameter `%s'.\n",
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
      log_err(parser, img,                                                     \
        "the image parameter `"Name"' is missing.\n");                         \
      res = RES_BAD_ARG;                                                       \
      goto error;                                                              \
    } (void)0
  CHECK_PARAM(PATH, "path");
  #undef CHECK_PARAM

exit:
  out_iimg->i = isolimg;
  return res;
error:
  if(solimg) {
    darray_image_pop_back(&parser->images);
    isolimg = SIZE_MAX;
  }
  goto exit;
}

