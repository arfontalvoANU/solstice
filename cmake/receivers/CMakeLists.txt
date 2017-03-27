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
project(srcvl C)

set(SRCVL_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../../src/receivers)

################################################################################
# Define include directories
################################################################################
include_directories(
  ${LibYAML_INCLUDE_DIR}
  ${RSys_INCLUDE_DIR}
  ${SRCVL_SOURCE_DIR}/../)

################################################################################
# Configure and define targets
################################################################################
set(SRCVL_FILES_SRC srcvl.c)
set(SRCVL_FILES_INC srcvl.h)

rcmake_prepend_path(SRCVL_FILES_SRC ${SRCVL_SOURCE_DIR})
rcmake_prepend_path(SRCVL_FILES_INC ${SRCVL_SOURCE_DIR})

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

add_library(srcvl STATIC ${SRCVL_FILES_SRC} ${SRCVL_FILES_INC})
target_link_libraries(srcvl LibYAML ${MATH_LIB})

################################################################################
# Tests
################################################################################
if(NOT NO_TEST)
  function(build_test _name)
    add_executable(${_name}
      ${SRCVL_SOURCE_DIR}/${_name}.c
      ${SRCVL_SOURCE_DIR}/../test_solstice_utils.h)
    target_link_libraries(${_name} LibYAML ${MATH_LIB} RSys srcvl)
  endfunction()

  function(new_test _name)
    build_test(${_name})
    add_test(${_name} ${_name})
  endfunction()

  build_test(test_srcvl)
  add_test(test_srcvl_ok test_srcvl
    ${SRCVL_SOURCE_DIR}/yaml/test_ok.yaml)
  add_test(test_srvvl_ko test_srcvl -e
    ${SRCVL_SOURCE_DIR}/yaml/test_ko.yaml)

  new_test(test_srcvl2)
  rcmake_copy_runtime_libraries(test_srcvl)

endif()


