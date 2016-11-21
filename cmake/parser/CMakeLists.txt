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
project(solstice-parser C CXX)

set(PARSER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src/parser)

################################################################################
# Define include directories
################################################################################
include_directories(
  ${LibYAML_INCLUDE_DIR}
  ${RSys_INCLUDE_DIR}
  ${PARSER_SOURCE_DIR}/../)

################################################################################
# Configure and define targets
################################################################################
set(PARSER_FILES_SRC
  solstice_parser.c)
set(PARSER_FILES_INC
  solstice_entity.h
  solstice_material.h
  solstice_parser.h
  solstice_shape.h
  solstice_sun.h)

# Prepend each file in the `SOLSTICE_FILES_<SRC|INC>' list by `SOLSTICE_SOURCE_DIR'
rcmake_prepend_path(PARSER_FILES_SRC ${PARSER_SOURCE_DIR})
rcmake_prepend_path(PARSER_FILES_INC ${PARSER_SOURCE_DIR})
rcmake_prepend_path(SOARSER_FILES_DOC ${PROJECT_SOURCE_DIR}/../)

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

add_library(solstice-parser STATIC ${PARSER_FILES_SRC} ${PARSER_FILES_INC})

################################################################################
# Tests
################################################################################
if(NOT NO_TEST)
  function(build_test _name)
    add_executable(${_name}
      ${PARSER_SOURCE_DIR}/${_name}.c
      ${PARSER_SOURCE_DIR}/../test_solstice_utils.h)
    target_link_libraries(${_name} LibYAML ${MATH_LIB} RSys solstice-parser)
  endfunction()

  function(new_test _name)
    build_test(${_name})
    add_test(${_name} ${_name})
  endfunction()

  new_test(test_solstice_parser2)

  build_test(test_solstice_parser)
  add_test(test_solstice_parser_ok_0 test_solstice_parser
    ${PARSER_SOURCE_DIR}/yaml/test_ok_0.yaml)
  add_test(test_solstice_parser_ok_1 test_solstice_parser
    ${PARSER_SOURCE_DIR}/yaml/test_ok_1.yaml)
  add_test(test_solstice_parser_ok_2 test_solstice_parser
    ${PARSER_SOURCE_DIR}/yaml/test_ok_2.yaml)
# TODO rewrite them wrt the update of the  input specification
#  add_test(test_solstice_parser_ko_0 test_solstice_parser -e
#    ${SOLSTICE_SOURCE_DIR}/yaml/test_ko_0.yaml)
#  add_test(test_solstice_parser_ko_1 test_solstice_parser -e
#    ${SOLSTICE_SOURCE_DIR}/yaml/test_ko_1.yaml)
#  add_test(test_solstice_parser_ko_2 test_solstice_parser -e
#    ${SOLSTICE_SOURCE_DIR}/yaml/test_ko_2.yaml)
endif()


