# Copyright (C) CNRS 2016
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
project(solstice-core C)
enable_testing()

set(CORE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src/core)

################################################################################
# Define include directories
################################################################################
include_directories(
  ${RSys_INCLUDE_DIR}
  ${SolAnim_INCLUDE_DIR}
  ${SolSolver_INCLUDE_DIR}
  ${CORE_SOURCE_DIR}/../)

################################################################################
# Configure and define targets
################################################################################
set(CORE_FILES_SRC
  solstice_core_device.c
  solstice_core_node.c
  solstice_core_scene.c
  solstice_core_solver.c)

set(CORE_FILES_INC
  solstice_core.h
  solstice_core_device.h
  solstice_core_node.h
  solstice_core_scene.h)

# Prepend each file in the `SOLSTICE_FILES_<SRC|INC>' list by `SOLSTICE_SOURCE_DIR'
rcmake_prepend_path(CORE_FILES_SRC ${CORE_SOURCE_DIR})
rcmake_prepend_path(CORE_FILES_INC ${CORE_SOURCE_DIR})
rcmake_prepend_path(CORE_FILES_DOC ${PROJECT_SOURCE_DIR}/../)

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

add_library(solstice-core STATIC ${CORE_FILES_SRC} ${CORE_FILES_INC})

################################################################################
# Tests
################################################################################
if(NOT NO_TEST)
  function(new_test _name)
    add_executable(${_name} ${CORE_SOURCE_DIR}/${_name}.c)
    target_link_libraries(${_name} RSys ${MATH_LIB} StarSP SolAnim SolSolver 
      solstice-core solstice-core-test)
    add_test(${_name} ${_name})
    rcmake_set_test_runtime_dirs(${_name} _runtime_dirs)
  endfunction()

  add_library(solstice-core-test 
    ${CORE_SOURCE_DIR}/test_core_utils.h
    ${CORE_SOURCE_DIR}/test_core_utils.c)

  
  new_test(test_core_device)
  new_test(test_core_node)
  new_test(test_core_scene)
  new_test(test_core_solve1)

  rcmake_copy_runtime_libraries(test_core_solve1)
endif()
