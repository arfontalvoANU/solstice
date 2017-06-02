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

#include "solparser_c.h"

/*******************************************************************************
 * Local function
 ******************************************************************************/
res_T
parse_mtl_data
  (struct solparser* parser,
   yaml_document_t* doc,
   yaml_node_t* mtl_data,
   const double lower_bound,
   const double upper_bound,
   struct solparser_mtl_data* data)
{
  res_T res = RES_OK;
  ASSERT(doc && mtl_data && data);

  if(mtl_data->type == YAML_SCALAR_NODE) {
    data->type = SOLPARSER_MTL_DATA_REAL;
    res = parse_real
      (parser, mtl_data, lower_bound, upper_bound, &data->value.real);
  } else if(mtl_data->type == YAML_SEQUENCE_NODE) {
    data->type = SOLPARSER_MTL_DATA_SPECTRUM;
    res = parse_spectrum
      (parser, doc, mtl_data, lower_bound, upper_bound, &data->value.spectrum);
  } else {
    log_err(parser, mtl_data, "expect a real or a spectrum definition.\n");
    res = RES_BAD_ARG;
    goto error;
  }
  if(res != RES_OK) {
    log_node(parser, mtl_data);
    goto error;
  }

exit:
  return res;
error:
  goto exit;
}

