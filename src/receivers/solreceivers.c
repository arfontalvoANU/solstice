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

#include "solreceivers.h"

#include <rsys/mem_allocator.h>
#include <rsys/ref_count.h>
#include <rsys/str.h>

#include <stdio.h>
#include <stdarg.h>
#include <yaml.h>

struct solreceivers {
  yaml_parser_t parser;
  int parser_is_init;

  struct str stream_name;

  ref_T ref;
  struct mem_allocator* allocator;
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static INLINE void
log_err
  (const struct solreceivers* receivers,
   const yaml_node_t* node,
   const char* fmt,
   ...)
{
  va_list vargs_list;
  ASSERT(parser && node && fmt);

  fprintf(stderr, "%s:%lu:%lu: ",
    str_cget(&receivers->stream_name),
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
  va_start(vargs_list, fmt);
  vfprintf(stderr, fmt, vargs_list);
  va_end(vargs_list);
}

static res_T
parse_receiver
  (struct solreceivers* receivers,
   yaml_document_t* doc,
   const yaml_node_t* receiver)
{
  ASSERT(receivers && doc && receiver);
  (void)receivers, (void)doc, (void)receiver;
  /* TODO */
  return RES_OK;
}

static void
receivers_clear(struct solreceivers* receivers)
{
  ASSERT(receivers);
  (void)receivers;
  /* TODO */
}

static void
receivers_release(ref_T* ref)
{
  struct solreceivers* receivers;
  ASSERT(ref);
  receivers = CONTAINER_OF(ref, struct solreceivers, ref);
  if(receivers->parser_is_init) yaml_parser_delete(&receivers->parser);
  str_release(&receivers->stream_name);
  MEM_RM(receivers->allocator, receivers);
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
solreceivers_create
  (struct mem_allocator* allocater, struct solreceivers** out_receivers)
{
  struct solreceivers* receivers = NULL;
  struct mem_allocator* mem_allocator;
  res_T res = RES_OK;
  ASSERT(out_receivers);

  mem_allocator = allocater ? allocater : &mem_default_allocator;
  receivers = MEM_CALLOC(mem_allocator, 1, sizeof(struct solreceivers));
  if(!receivers) {
    fprintf(stderr, "Could not allocate the loader of the receivers.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  receivers->allocator = mem_allocator;
  ref_init(&receivers->ref);
  str_init(mem_allocator, &receivers->stream_name);

exit:
  *out_receivers = receivers;
  return res;
error:
  if(receivers) {
    solreceivers_ref_put(receivers);
    receivers = NULL;
  }
  goto exit;
}

void
solreceivers_ref_get(struct solreceivers* receivers)
{
  ASSERT(receivers);
  ref_get(&receivers->ref);
}

void
solreceivers_ref_put(struct solreceivers* receivers)
{
  ASSERT(receivers);
  ref_put(&receivers->ref, receivers_release);
}

res_T
solreceivers_setup_stream
  (struct solreceivers* receivers,
   const char* stream_name,
   FILE* stream)
{
  res_T res = RES_OK;
  ASSERT(receivers && stream);

  if(receivers->parser_is_init) {
    yaml_parser_delete(&receivers->parser);
    receivers->parser_is_init = 0;
  }

  res = str_set(&receivers->stream_name, stream_name ? stream_name:"<stream>");
  if(res != RES_OK) {
    fprintf(stderr, "Could not register the filename of the receiver stream.\n");
    goto error;
  }
  if(!yaml_parser_initialize(&receivers->parser)) {
    fprintf(stderr,
      "Could not initialise the YAML parser of the receiver stream.\n");
    res = RES_UNKNOWN_ERR;
    goto error;
  }
  receivers->parser_is_init = 1;
  yaml_parser_set_input_file(&receivers->parser, stream);

exit:
  return res;
error:
  str_clear(&receivers->stream_name);
  if(receivers->parser_is_init) {
    yaml_parser_delete(&receivers->parser);
    receivers->parser_is_init = 0;
  }
  goto exit;
}

res_T
solreceivers_load(struct solreceivers* receivers)
{
  yaml_document_t doc;
  yaml_node_t* root;
  const char* stream_name;
  intptr_t i, n;
  int doc_is_init = 0;
  res_T res = RES_OK;
  ASSERT(receivers);

  stream_name = str_cget(&receivers->stream_name);
  receivers_clear(receivers); /* Clean up previously loaded data */

  if(!receivers->parser_is_init) {
    res = RES_BAD_OP;
    goto error;
  }

  if(!yaml_parser_load(&receivers->parser, &doc)) {
    fprintf(stderr, "%s:%lu:%lu: %s.\n",
      stream_name,
      (unsigned long)receivers->parser.problem_mark.line+1,
      (unsigned long)receivers->parser.problem_mark.column+1,
      receivers->parser.problem);
    yaml_parser_delete(&receivers->parser);
    receivers->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }
  doc_is_init = 1;

  root = yaml_document_get_root_node(&doc);
  if(!root) {
    yaml_parser_delete(&receivers->parser);
    receivers->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }

  if(root->type != YAML_SEQUENCE_NODE) {
    log_err(receivers, root, "expect a list of receivers.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = root->data.sequence.items.top - root->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* receiver;

    receiver = yaml_document_get_node(&doc, root->data.sequence.items.start[i]);
    res = parse_receiver(receivers, &doc, receiver);
    if(res != RES_OK) goto error;
  }
exit:
  if(doc_is_init) yaml_document_delete(&doc);
  return res;
error:
  receivers_clear(receivers);
  goto exit;
}

