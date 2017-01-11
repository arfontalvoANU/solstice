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

#ifndef SOLRECEIVERS_H
#define SOLRECEIVERS_H

#include <rsys/rsys.h>

struct mem_allocator;
struct solreceivers;

/*******************************************************************************
 * Solstice Receiver API
 ******************************************************************************/
extern LOCAL_SYM res_T
solreceivers_create
  (struct mem_allocator* allocator, /* May be NULL <=> use default allocator */
   struct solreceivers** receivers);

extern LOCAL_SYM void
solreceivers_ref_get
  (struct solreceivers* receivers);

extern LOCAL_SYM void
solreceivers_ref_put
  (struct solreceivers* receivers);

extern LOCAL_SYM res_T
solreceivers_setup_stream
  (struct solreceivers* receivers,
   const char* stream_name, /* May be NULL */
   FILE* stream);

/* Return RES_BAD_OP if there is no more YAML document to parse */
extern LOCAL_SYM res_T
solreceivers_load
  (struct solreceivers* receivers);

#endif /* SOLRECEIVERS_H */

