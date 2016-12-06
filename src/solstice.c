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
 * Helper functions
 ******************************************************************************/
static void
clear_materials(struct htable_material* materials)
{
  struct htable_material_iterator it, end;
  ASSERT(materials);

  htable_material_begin(materials, &it);
  htable_material_end(materials, &end);
  while(!htable_material_iterator_eq(&it, &end)) {
    struct ssol_material* mtl = *htable_material_iterator_data_get(&it);
    SSOL(material_ref_put(mtl));
    htable_material_iterator_next(&it);
  }
  htable_material_clear(materials);
}

static void
clear_objects(struct htable_object* objects)
{
  struct htable_object_iterator it, end;
  ASSERT(objects);

  htable_object_begin(objects, &it);
  htable_object_end(objects, &end);
  while(!htable_object_iterator_eq(&it, &end)) {
    struct ssol_object* obj = *htable_object_iterator_data_get(&it);
    SSOL(object_ref_put(obj));
    htable_object_iterator_next(&it);
  }
  htable_object_clear(objects);
}

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

  solstice->allocator = allocator ? allocator : &mem_default_allocator;

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
  clear_materials(&solstice->materials);
  clear_objects(&solstice->objects);
  if(solstice->ssol) SSOL(device_ref_put(solstice->ssol));
  if(solstice->parser) solparser_ref_put(solstice->parser);
  htable_material_release(&solstice->materials);
  htable_object_release(&solstice->objects);
}

