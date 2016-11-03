/* Copyright (C) CNRS 2016
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

#include "solstice_facility.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <yaml.h>

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static INLINE void
log_err
  (const char* filename,
   const yaml_node_t* node,
   const char* fmt,
   ...)
{
  va_list vargs_list;
  ASSERT(node && fmt);

  fprintf(stderr, "%s:%lu:%lu: ",
    filename,
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
  va_start(vargs_list, fmt);
  vfprintf(stderr, fmt, vargs_list);
  va_end(vargs_list);
}

static res_T
parse_item
  (const char* filename,
   yaml_document_t* doc,
   const yaml_node_t* item)
{
  yaml_node_t* key;
  yaml_node_t* val;
  intptr_t nattrs;
  res_T res = RES_OK;
  ASSERT(item && item->type == YAML_MAPPING_NODE);

  nattrs = item->data.mapping.pairs.top - item->data.mapping.pairs.start;
  if(nattrs != 1) {
    log_err(filename, item,
      "expecting only one `key : value' pair while %lu are submitted.\n",
      (unsigned long)nattrs);
    res = RES_BAD_ARG;
    goto error;
  }

  key = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].key);
  val = yaml_document_get_node(doc, item->data.mapping.pairs.start[0].value);
  if(key->type != YAML_SCALAR_NODE) {
    log_err(filename, key, "expecting a scalar YAML value.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  (void)val;

  if(!strcmp((char*)key->data.scalar.value, "material")) { /* TODO */
  } else if(!strcmp((char*)key->data.scalar.value, "node")) { /* TODO */
  } else if(!strcmp((char*)key->data.scalar.value, "object")) { /* TODO */
  } else if(!strcmp((char*)key->data.scalar.value, "pivot")) { /* TODO */
  } else if(!strcmp((char*)key->data.scalar.value, "spawn")) { /* TODO */
  } else {
    log_err(filename, key, "unknown item `%s'.\n", key->data.scalar.value);
    res = RES_BAD_ARG;
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
solstice_facility_load(const char* filename)
{
  yaml_parser_t parser;
  yaml_document_t doc;
  yaml_node_t* root;
  FILE* file = NULL;
  size_t i, n;
  int doc_is_init;
  res_T res = RES_OK;

  if(!yaml_parser_initialize(&parser)) {
    fprintf(stderr, "Could not initialise the YAML parser.\n");
    return RES_UNKNOWN_ERR;
  }

  file = fopen(filename, "rb");
  if(!file) {
    fprintf(stderr, "Could not open the YAML file `%s'.\n", filename);
    res = RES_IO_ERR;
    goto error;
  }

  yaml_parser_set_input_file(&parser, file);

  if(!yaml_parser_load(&parser, &doc)) {
    fprintf(stderr, "%s:%lu:%lu: %s.\n",
      filename,
      (unsigned long)parser.problem_mark.line+1,
      (unsigned long)parser.problem_mark.column+1,
      parser.problem);
    res = RES_IO_ERR;
    goto error;
  }
  doc_is_init = 1;

  root = yaml_document_get_root_node(&doc);
  if(root->type != YAML_SEQUENCE_NODE) {
    log_err(filename, root, "expecting a YAML sequence.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = (size_t)(root->data.sequence.items.top - root->data.sequence.items.start);
  FOR_EACH(i, 0, n) {
    yaml_node_t* item;

    item = yaml_document_get_node(&doc, root->data.sequence.items.start[i]);
    if(item->type != YAML_MAPPING_NODE) {
      log_err(filename, item, "expecting a YAML mapping.\n");
      res = RES_BAD_ARG;
      goto error;
    }

    res = parse_item(filename, &doc, item);
    if(res != RES_OK) goto error;
  }

exit:
  yaml_parser_delete(&parser);
  if(doc_is_init) yaml_document_delete(&doc);
  if(file) fclose(file);
  return res;
error:
  goto exit;
}

