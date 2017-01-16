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

#ifndef SRCVL_H /* Solstice ReCeiVers Loader */
#define SRCVL_H

#include <rsys/rsys.h>

enum srcvl_side {
  SRCVL_FRONT,
  SRCVL_BACK,
  SRCVL_FRONT_AND_BACK
};

struct srcvl_receiver {
  const char* name;
  enum srcvl_side side;
};

struct mem_allocator;
struct srcvl;

/*******************************************************************************
 * Solstice Receiver API
 ******************************************************************************/
extern LOCAL_SYM res_T
srcvl_create
  (struct mem_allocator* allocator, /* May be NULL <=> use default allocator */
   struct srcvl** rcvl);

extern LOCAL_SYM void
srcvl_ref_get
  (struct srcvl* rcvl);

extern LOCAL_SYM void
srcvl_ref_put
  (struct srcvl* rcvl);

extern LOCAL_SYM res_T
srcvl_setup_stream
  (struct srcvl* rcvl,
   const char* stream_name, /* May be NULL */
   FILE* stream);

/* Return RES_BAD_OP if there is no more YAML document to parse */
extern LOCAL_SYM res_T
srcvl_load
  (struct srcvl* rcvl);

extern LOCAL_SYM size_t
srcvl_count
  (const struct srcvl* rcvl);

extern LOCAL_SYM void
srcvl_get
  (const struct srcvl* rcvl,
   const size_t i,
   struct srcvl_receiver* receiver);

#endif /* SRCVL_H */

