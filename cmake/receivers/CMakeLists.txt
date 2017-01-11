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
project(solstice-receivers C)

set(SOLRECEIVERS_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src/receivers)

################################################################################
# Define include directories
################################################################################
include_directories(
  ${LibYAML_INCLUDE_DIR}
  ${RSys_INCLUDE_DIR}
  ${SOLRECEIVERS_SOURCE_DIR}/../)

################################################################################
# Configure and define targets
################################################################################
set(SOLRECEIVERS_FILES_SRC solreceivers.c)
set(SOLRECEIVERS_FILES_INC solreceivers.h)

rcmake_prepend_path(SOLRECEIVERS_FILES_SRC ${SOLRECEIVERS_SOURCE_DIR})
rcmake_prepend_path(SOLRECEIVERS_FILES_INC ${SOLRECEIVERS_SOURCE_DIR})

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

add_library(solreceivers STATIC ${SOLRECEIVERS_FILES_SRC} ${SOLRECEIVERS_FILES_INC})
target_link_libraries(solreceivers ${MATH_LIB})

################################################################################
# Tests
################################################################################
if(NOT NO_TEST)
  function(build_test _name)
    add_executable(${_name}
      ${SOLRECEIVERS_SOURCE_DIR}/${_name}.c
      ${SOLRECEIVERS_SOURCE_DIR}/../test_solstice_utils.h)
    target_link_libraries(${_name} LibYAML ${MATH_LIB} RSys solparser)
  endfunction()

  function(new_test _name)
    build_test(${_name})
    add_test(${_name} ${_name})
  endfunction()

endif()


