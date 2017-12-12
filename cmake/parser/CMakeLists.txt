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
project(solstice-parser C)

set(SOLPARSER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src/parser)

################################################################################
# Define include directories
################################################################################
include_directories(
  ${LibYAML_INCLUDE_DIR}
  ${RSys_INCLUDE_DIR}
  ${SOLPARSER_SOURCE_DIR}/../)

################################################################################
# Configure and define targets
################################################################################
set(SOLPARSER_FILES_SRC
  solparser.c
  solparser_atmosphere.c
  solparser_entity.c
  solparser_image.c
  solparser_geometry.c
  solparser_material.c
  solparser_medium.c
  solparser_mtl_data.c
  solparser_pivot.c
  solparser_sun.c
  solparser_spectrum.c)
set(SOLPARSER_FILES_INC
  solparser.h
  solparser_c.h
  solparser_atmosphere.h
  solparser_entity.h
  solparser_geometry.h
  solparser_image.h
  solparser_material.h
  solparser_medium.h
  solparser_mtl_data.h
  solparser_pivot.h
  solparser_shape.h
  solparser_sun.h
  solparser_spectrum.h)

# Prepend each file in the `SOLPARSER_FILES_<SRC|INC>' list by `SOLPARSER_SOURCE_DIR'
rcmake_prepend_path(SOLPARSER_FILES_SRC ${SOLPARSER_SOURCE_DIR})
rcmake_prepend_path(SOLPARSER_FILES_INC ${SOLPARSER_SOURCE_DIR})
rcmake_prepend_path(SOLPARSER_FILES_DOC ${PROJECT_SOURCE_DIR}/../)

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

add_library(solparser STATIC ${SOLPARSER_FILES_SRC} ${SOLPARSER_FILES_INC})
target_link_libraries(solparser LibYAML ${MATH_LIB})

################################################################################
# Tests
################################################################################
if(NOT NO_TEST)
  function(build_test _name)
    add_executable(${_name}
      ${SOLPARSER_SOURCE_DIR}/${_name}.c
      ${SOLPARSER_SOURCE_DIR}/../test_solstice_utils.h)
    target_link_libraries(${_name} LibYAML ${MATH_LIB} RSys solparser)
  endfunction()

  function(new_test _name)
    build_test(${_name})
    add_test(${_name} ${_name})
  endfunction()

  build_test(test_solparser)
  add_test(test_solparser_ok_0 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_0.yaml)
  add_test(test_solparser_ok_1 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_1.yaml)
  add_test(test_solparser_ok_2 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_2.yaml)
  add_test(test_solparser_ok_3 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_3.yaml)
  add_test(test_solparser_ok_4 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_4.yaml)
  add_test(test_solparser_ok_5 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_5.yaml)
  add_test(test_solparser_ok_6 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_6.yaml)
  add_test(test_solparser_ok_7 test_solparser
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ok_7.yaml)
  add_test(test_solparser_ko test_solparser -e
    ${SOLPARSER_SOURCE_DIR}/yaml/test_ko_0.yaml)

  new_test(test_solparser2)
  new_test(test_solparser3)
  new_test(test_solparser4)
  new_test(test_solparser5)
  new_test(test_solparser6)
  new_test(test_solparser7)
  new_test(test_solparser8)
  new_test(test_solparser_normal_map)
  new_test(test_solparser_spectrum)
  new_test(test_solparser_mirror)

  rcmake_copy_runtime_libraries(test_solparser)
endif()

