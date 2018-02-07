/* Copyright (C) CNRS 2016-2018
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

#include "srcvl.h"

#include <rsys/cstr.h>
#include <rsys/dynamic_array.h>
#include <rsys/mem_allocator.h>
#include <rsys/ref_count.h>
#include <rsys/str.h>

#include <stdio.h>
#include <stdarg.h>
#include <yaml.h>

struct receiver {
  struct str name;
  enum srcvl_side side;
  int per_primitive;
};

static INLINE void
receiver_init(struct mem_allocator* allocator, struct receiver* receiver)
{
  ASSERT(receiver);
  str_init(allocator, &receiver->name);
  receiver->side = SRCVL_FRONT_AND_BACK;
  receiver->per_primitive = 0;
}

static INLINE void
receiver_release(struct receiver* receiver)
{
  ASSERT(receiver);
  str_release(&receiver->name);
}

static INLINE res_T
receiver_copy(struct receiver* dst, const struct receiver* src)
{
  ASSERT(dst && src);
  dst->side = src->side;
  dst->per_primitive = src->per_primitive;
  return str_copy(&dst->name, &src->name);
}

static INLINE res_T
receiver_copy_and_release(struct receiver* dst, struct receiver* src)
{
  ASSERT(dst && src);
  dst->side = src->side;
  dst->per_primitive = src->per_primitive;
  return str_copy_and_release(&dst->name, &src->name);
}

#define DARRAY_NAME receiver
#define DARRAY_DATA struct receiver
#define DARRAY_FUNCTOR_INIT receiver_init
#define DARRAY_FUNCTOR_RELEASE receiver_release
#define DARRAY_FUNCTOR_COPY receiver_copy
#define DARRAY_FUNCTOR_COPY_AND_RELEASE receiver_copy_and_release
#include <rsys/dynamic_array.h>

struct srcvl {
  yaml_parser_t parser;
  int parser_is_init;
  struct darray_receiver receivers;

  struct str stream_name;

  ref_T ref;
  struct mem_allocator* allocator;
};

/*******************************************************************************
 * Helper functions
 ******************************************************************************/
static INLINE void
log_err
  (const struct srcvl* srcvl,
   const yaml_node_t* node,
   const char* fmt,
   ...)
{
  va_list vargs_list;
  ASSERT(srcvl && node && fmt);

  fprintf(stderr, "%s:%lu:%lu: ",
    str_cget(&srcvl->stream_name),
    (unsigned long)node->start_mark.line+1,
    (unsigned long)node->start_mark.column+1);
  va_start(vargs_list, fmt);
  vfprintf(stderr, fmt, vargs_list);
  va_end(vargs_list);
}

static res_T
parse_string
  (struct srcvl* srcvl,
   yaml_node_t* string,
   struct str* str)
{
  res_T res = RES_OK;
  ASSERT(string && str);

  if(string->type != YAML_SCALAR_NODE
  || !strlen((char*)string->data.scalar.value)) {
    log_err(srcvl, string, "expect a character string.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  res = str_set(str, (char*)string->data.scalar.value);
  if(res !=  RES_OK) {
    log_err(srcvl, string, "could not register the string `%s'.\n",
      string->data.scalar.value);
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_side
  (struct srcvl* srcvl,
   yaml_node_t* side,
   enum srcvl_side* out_side)
{
  res_T res = RES_OK;
  ASSERT(side && out_side);

  if(side->type != YAML_SCALAR_NODE) {
    log_err(srcvl, side, "expect a character string.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  if(!strcmp((char*)side->data.scalar.value, "FRONT")) {
    *out_side = SRCVL_FRONT;
  } else if(!strcmp((char*)side->data.scalar.value, "BACK")) {
    *out_side = SRCVL_BACK;
  } else if(!strcmp((char*)side->data.scalar.value, "FRONT_AND_BACK")) {
    *out_side = SRCVL_FRONT_AND_BACK;
  } else {
    log_err(srcvl, side, "unknown side value `%s'.\n",
      side->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }
exit:
  return res;
error:
  goto exit;
}

static res_T
parse_integer(struct srcvl* srcvl, yaml_node_t* integer, int* dst)
{
  res_T res = RES_OK;
  ASSERT(integer && dst);

  if(integer->type != YAML_SCALAR_NODE
  || !strlen((char*)integer->data.scalar.value)) {
    log_err(srcvl, integer, "expect an integer.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  res = cstr_to_int((char*)integer->data.scalar.value, dst);
  if(res != RES_OK) {
    log_err(srcvl, integer, "invalid integer `%s'.\n",
      integer->data.scalar.value);
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

static res_T
parse_receiver
  (struct srcvl* srcvl,
   yaml_document_t* doc,
   const yaml_node_t* receiver)
{
  enum { NAME, PER_PRIMITIVE, SIDE };
  struct receiver* solreceiver = NULL;
  size_t isolreceiver;
  intptr_t i, n;
  int mask = 0; /* Register the parsed attributes */
  res_T res = RES_OK;
  ASSERT(srcvl && doc && receiver);

  if(receiver->type != YAML_MAPPING_NODE) {
    log_err(srcvl, receiver, "expect a receiver definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  /* Allocate the receiver */
  isolreceiver = darray_receiver_size_get(&srcvl->receivers);
  res = darray_receiver_resize(&srcvl->receivers, isolreceiver + 1);
  if(res != RES_OK) {
    log_err(srcvl, receiver, "could not allocate the receiver.\n");
    goto error;
  }
  solreceiver = darray_receiver_data_get(&srcvl->receivers) + isolreceiver;

  n = receiver->data.mapping.pairs.top - receiver->data.mapping.pairs.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* key;
    yaml_node_t* val;

    key = yaml_document_get_node(doc, receiver->data.mapping.pairs.start[i].key);
    val = yaml_document_get_node(doc, receiver->data.mapping.pairs.start[i].value);
    if(key->type != YAML_SCALAR_NODE) {
      log_err(srcvl, key, "expect receiver parameters.\n");
      res = RES_BAD_ARG;
      goto error;
    }
    #define SETUP_MASK(Flag, Name) {                                           \
      if(mask & BIT(Flag)) {                                                   \
        log_err(srcvl, key, "the receiver "Name" is already defined.\n");  \
        res = RES_BAD_ARG;                                                     \
        goto error;                                                            \
      }                                                                        \
      mask |= BIT(Flag);                                                       \
    } (void)0
    if(!strcmp((char*)key->data.scalar.value, "name")) {
      SETUP_MASK(NAME, "name");
      res = parse_string(srcvl, val, &solreceiver->name);
    } else if(!strcmp((char*)key->data.scalar.value, "side")) {
      SETUP_MASK(SIDE, "side");
      res = parse_side(srcvl, val, &solreceiver->side);
    } else if(!strcmp((char*)key->data.scalar.value, "per_primitive")) {
      SETUP_MASK(PER_PRIMITIVE, "per_primitive");
      res = parse_integer(srcvl, val, &solreceiver->per_primitive);
    } else {
      log_err(srcvl, key, "unknown receiver parameter `%s'.\n",
        key->data.scalar.value);
      res = RES_BAD_ARG;
    }
    if(res != RES_OK) goto error;
    #undef SETUP_MASK
  }

  if(!(mask & BIT(NAME))) {
    log_err(srcvl, receiver, "the receiver name is missing.\n");
    res = RES_BAD_ARG;
    goto error;
  }

exit:
  return res;
error:
  if(solreceiver) darray_receiver_pop_back(&srcvl->receivers);
  goto exit;
}

static void
receivers_clear(struct srcvl* srcvl)
{
  ASSERT(srcvl);
  darray_receiver_clear(&srcvl->receivers);
}

static void
receivers_release(ref_T* ref)
{
  struct srcvl* srcvl;
  ASSERT(ref);
  srcvl = CONTAINER_OF(ref, struct srcvl, ref);
  if(srcvl->parser_is_init) yaml_parser_delete(&srcvl->parser);
  str_release(&srcvl->stream_name);
  darray_receiver_release(&srcvl->receivers);
  MEM_RM(srcvl->allocator, srcvl);
}

/*******************************************************************************
 * Local functions
 ******************************************************************************/
res_T
srcvl_create
  (struct mem_allocator* allocater, struct srcvl** out_receivers)
{
  struct srcvl* srcvl = NULL;
  struct mem_allocator* mem_allocator;
  res_T res = RES_OK;
  ASSERT(out_receivers);

  mem_allocator = allocater ? allocater : &mem_default_allocator;
  srcvl = MEM_CALLOC(mem_allocator, 1, sizeof(struct srcvl));
  if(!srcvl) {
    fprintf(stderr, "Could not allocate the loader of the srcvl.\n");
    res = RES_MEM_ERR;
    goto error;
  }
  srcvl->allocator = mem_allocator;
  ref_init(&srcvl->ref);
  str_init(mem_allocator, &srcvl->stream_name);
  darray_receiver_init(mem_allocator, &srcvl->receivers);

exit:
  *out_receivers = srcvl;
  return res;
error:
  if(srcvl) {
    srcvl_ref_put(srcvl);
    srcvl = NULL;
  }
  goto exit;
}

void
srcvl_ref_get(struct srcvl* srcvl)
{
  ASSERT(srcvl);
  ref_get(&srcvl->ref);
}

void
srcvl_ref_put(struct srcvl* srcvl)
{
  ASSERT(srcvl);
  ref_put(&srcvl->ref, receivers_release);
}

res_T
srcvl_setup_stream
  (struct srcvl* srcvl,
   const char* stream_name,
   FILE* stream)
{
  res_T res = RES_OK;
  ASSERT(srcvl && stream);

  if(srcvl->parser_is_init) {
    yaml_parser_delete(&srcvl->parser);
    srcvl->parser_is_init = 0;
  }

  res = str_set(&srcvl->stream_name, stream_name ? stream_name:"<stream>");
  if(res != RES_OK) {
    fprintf(stderr, "Could not register the filename of the receiver stream.\n");
    goto error;
  }
  if(!yaml_parser_initialize(&srcvl->parser)) {
    fprintf(stderr,
      "Could not initialise the YAML parser of the receiver stream.\n");
    res = RES_UNKNOWN_ERR;
    goto error;
  }
  srcvl->parser_is_init = 1;
  yaml_parser_set_input_file(&srcvl->parser, stream);

exit:
  return res;
error:
  str_clear(&srcvl->stream_name);
  if(srcvl->parser_is_init) {
    yaml_parser_delete(&srcvl->parser);
    srcvl->parser_is_init = 0;
  }
  goto exit;
}

res_T
srcvl_load(struct srcvl* srcvl)
{
  yaml_document_t doc;
  yaml_node_t* root;
  const char* stream_name;
  intptr_t i, n;
  int doc_is_init = 0;
  res_T res = RES_OK;
  ASSERT(srcvl);

  stream_name = str_cget(&srcvl->stream_name);
  receivers_clear(srcvl); /* Clean up previously loaded data */

  if(!srcvl->parser_is_init) {
    res = RES_BAD_OP;
    goto error;
  }

  if(!yaml_parser_load(&srcvl->parser, &doc)) {
    fprintf(stderr, "%s:%lu:%lu: %s.\n",
      stream_name,
      (unsigned long)srcvl->parser.problem_mark.line+1,
      (unsigned long)srcvl->parser.problem_mark.column+1,
      srcvl->parser.problem);
    yaml_parser_delete(&srcvl->parser);
    srcvl->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }
  doc_is_init = 1;

  root = yaml_document_get_root_node(&doc);
  if(!root) {
    yaml_parser_delete(&srcvl->parser);
    srcvl->parser_is_init = 0;
    res = RES_BAD_OP;
    goto error;
  }

  if(root->type != YAML_SEQUENCE_NODE) {
    log_err(srcvl, root, "expect a list of srcvl.\n");
    res = RES_BAD_ARG;
    goto error;
  }

  n = root->data.sequence.items.top - root->data.sequence.items.start;
  FOR_EACH(i, 0, n) {
    yaml_node_t* receiver;

    receiver = yaml_document_get_node(&doc, root->data.sequence.items.start[i]);
    res = parse_receiver(srcvl, &doc, receiver);
    if(res != RES_OK) goto error;
  }
exit:
  if(doc_is_init) yaml_document_delete(&doc);
  return res;
error:
  receivers_clear(srcvl);
  goto exit;
}

size_t
srcvl_count(const struct srcvl* srcvl)
{
  ASSERT(srcvl);
  return darray_receiver_size_get(&srcvl->receivers);
}

void
srcvl_get
  (const struct srcvl* srcvl,
   const size_t i,
   struct srcvl_receiver* receiver)
{
  const struct receiver* r;
  ASSERT(srcvl && receiver && i < srcvl_count(srcvl));
  r = darray_receiver_cdata_get(&srcvl->receivers) + i;
  receiver->name = str_cget(&r->name);
  receiver->side = r->side;
  receiver->per_primitive = r->per_primitive;
}

