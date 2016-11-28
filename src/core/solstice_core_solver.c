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

#include "solstice_core.h"
#include "solstice_core_scene.h"

#include <solstice/ssol.h>

res_T
score_solve
  (struct score_scene* scene,
   struct ssp_rng* rng,
   const size_t realisations_count,
   FILE* output,
   struct ssol_estimator* estimator)
{
  ASSERT(scene && rng  && realisations_count && output && estimator);
  return ssol_solve(scene->solver, rng, realisations_count, output, estimator);
}