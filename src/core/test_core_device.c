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

#include "solstice_core.h"
#include "test_core_utils.h"
#include "test_solstice_utils.h"

#include <rsys/logger.h>

int
main(int argc, char** argv)
{
  struct logger logger;
  struct mem_allocator allocator;
  struct score_device* dev;
  (void) argc, (void) argv;

  CHECK(score_device_create(NULL, NULL, 1, 0, &dev), RES_OK);
  score_device_ref_put(dev);

  mem_init_proxy_allocator(&allocator, &mem_default_allocator);

  CHECK(MEM_ALLOCATED_SIZE(&allocator), 0);
  CHECK(score_device_create(NULL, &allocator, 1, 0, &dev), RES_OK);
  score_device_ref_put(dev);
  CHECK(MEM_ALLOCATED_SIZE(&allocator), 0);

  CHECK(logger_init(&allocator, &logger), RES_OK);
  logger_set_stream(&logger, LOG_OUTPUT, log_stream, NULL);
  logger_set_stream(&logger, LOG_ERROR, log_stream, NULL);
  logger_set_stream(&logger, LOG_WARNING, log_stream, NULL);

  CHECK(score_device_create(&logger, NULL, 1, 0, &dev), RES_OK);
  score_device_ref_put(dev);

  CHECK(score_device_create(&logger, &allocator, 1, 0, &dev), RES_OK);
  score_device_ref_put(dev);

  CHECK(score_device_create(&logger, &allocator, 1, 0, &dev), RES_OK);
  score_device_ref_put(dev);

  logger_release(&logger);
  check_memory_allocator(&allocator);
  mem_shutdown_proxy_allocator(&allocator);
  CHECK(mem_allocated_size(), 0);
  return 0;
}
