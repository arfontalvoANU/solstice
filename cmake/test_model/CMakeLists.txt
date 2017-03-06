# Copyright (C) CNRS 2016-2017
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program. If not, see <http://www.gnu.org/licenses/>.

cmake_minimum_required(VERSION 2.8)
project(test_model C)

set(TESTMODEL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src/test_model)

################################################################################
# Define include directories
################################################################################
include_directories(
  ${LibYAML_INCLUDE_DIR}
  ${RSys_INCLUDE_DIR}
  ${TESTMODEL_SOURCE_DIR}/../)

################################################################################
# Configure and define targets
################################################################################
set(TESTMODEL_FILES_SRC
  test_model.c)

# Prepend each file in the `TESTMODEL_FILES_<SRC|INC>' list by `TESTMODEL_SOURCE_DIR'
rcmake_prepend_path(TESTMODEL_FILES_SRC ${TESTMODEL_SOURCE_DIR})

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

add_executable(test_model ${TESTMODEL_FILES_SRC})

################################################################################
# Tests
################################################################################
if(NOT NO_TEST)
  add_test(test_beam_down test_model beam_down)

  rcmake_copy_runtime_libraries(test_model)

endif()
