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

#include "solstice.h"
#include "parser/solparser.h"

#include <solstice/ssol.h>

/*******************************************************************************
 * Solstice local functions
 ******************************************************************************/
res_T
solstice_init(struct mem_allocator* allocator, struct solstice* solstice)
{
  res_T res = RES_OK;
  ASSERT(solstice);

  memset(solstice, 0, sizeof(struct solstice));
  htable_material_init(allocator, &solstice->materials);
  htable_object_init(allocator, &solstice->objects);

  res = ssol_device_create
    (NULL, allocator, SSOL_NTHREADS_DEFAULT, 0, &solstice->ssol);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Solver device.\n");
    goto error;
  }

  res = solparser_create(allocator, &solstice->parser);
  if(res != RES_OK) {
    fprintf(stderr, "Could not create the Solstice Parser.\n");
    goto error;
  }

exit:
  return res;
error:
  solstice_release(solstice);
  goto exit;
}

void
solstice_release(struct solstice* solstice)
{
  ASSERT(solstice);
  if(solstice->ssol) SSOL(device_ref_put(solstice->ssol));
  if(solstice->parser) solparser_ref_put(solstice->parser);
  htable_material_release(&solstice->materials);
  htable_object_release(&solstice->objects);
}

