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
project(solstice C)
enable_testing()

option(NO_TEST "Do not build tests" OFF)

if(CMAKE_HOST_UNIX)
  set(SOLSTICE_DOC "TROFF & HTML" CACHE STRING
    "Type of documentation to generate and install.")
else()
  set(SOLSTICE_DOC "HTML" CACHE STRING
    "Type of documentation to generate and install.")
endif()

set_property(CACHE SOLSTICE_DOC PROPERTY STRINGS
  "HTML"
  "TROFF"
  "TROFF & HTML"
  "NONE")

set(SOLSTICE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../src/)

################################################################################
# Check dependencies
################################################################################
get_filename_component(_current_source_dir ${CMAKE_CURRENT_LIST_FILE} PATH)
set(LibYAML_DIR ${_current_source_dir}/)

find_package(LibYAML REQUIRED)
find_package(RCMake 0.2.3 REQUIRED)
find_package(RSys 0.4 REQUIRED)
find_package(SolAnim 0.1 REQUIRED)
find_package(SolSolver 0.1.1 REQUIRED)
find_package(Star3DUT 0.2 REQUIRED)
find_package(StarSP 0.4 REQUIRED)
find_package(StarSTL 0.3 REQUIRED)

if(MSVC)
  find_package(MuslGetopt REQUIRED)
  include_directories(${MuslGetopt_INCLUDE_DIR})
endif()

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${RCMAKE_SOURCE_DIR})
include(rcmake)
include(rcmake_runtime)

include_directories(
  ${RSys_INCLUDE_DIR}
  ${MuslGetopt_INCLUDE_DIR}
  ${SolAnim_INCLUDE_DIR}
  ${SolSolver_INCLUDE_DIR}
  ${StarSP_INCLUDE_DIR}
  ${StarSTL_INCLUDE_DIR}
  ${Star3DUT_INCLUDE_DIR}
  ${CMAKE_CURRENT_BINARY_DIR})

################################################################################
# Build subprojects
################################################################################
if(NOT SOLSTICE_DOC STREQUAL "NONE")
  add_subdirectory(doc)
endif()
add_subdirectory(parser)
add_subdirectory(receivers)

################################################################################
# Generate files
################################################################################
set(SOLSTICE_ARGS_DEFAULT_NREALISATIONS "10000")
set(SOLSTICE_ARGS_DEFAULT_CAMERA_POS "0,0,0")
set(SOLSTICE_ARGS_DEFAULT_CAMERA_TGT "0,0,-1")
set(SOLSTICE_ARGS_DEFAULT_CAMERA_UP "0,1,0")
set(SOLSTICE_ARGS_DEFAULT_CAMERA_FOV "70") # In degrees
set(SOLSTICE_ARGS_DEFAULT_IMG_WIDTH  "800")
set(SOLSTICE_ARGS_DEFAULT_IMG_HEIGHT "600")
set(SOLSTICE_ARGS_DEFAULT_IMG_SPP "1")

configure_file(${SOLSTICE_SOURCE_DIR}/solstice_args.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/solstice_args.h @ONLY)
configure_file(${SOLSTICE_SOURCE_DIR}/../doc/solstice.1.txt.in
  ${CMAKE_CURRENT_BINARY_DIR}/doc/solstice.1.txt @ONLY)

################################################################################
# Configure and define targets
################################################################################
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 1)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(SOLSTICE_FILES_SRC
  solstice.c
  solstice_args.c
  solstice_atmosphere.c
  solstice_draw.c
  solstice_dump.c
  solstice_entity.c
  solstice_material.c
  solstice_node.c
  solstice_object.c
  solstice_solve.c
  solstice_spectrum.c
  solstice_sun.c
  solstice_sun_spectrum.c)
set(SOLSTICE_FILES_INC
  solstice.h
  solstice_args.h.in
  solstice_c.h
  solstice_sun_spectrum.h)
set(SOLSTICE_FILES_DOC COPYING README.md)

# Prepend each file in the `SOLSTICE_FILES_<SRC|INC>' list by `SOLSTICE_SOURCE_DIR'
rcmake_prepend_path(SOLSTICE_FILES_SRC ${SOLSTICE_SOURCE_DIR})
rcmake_prepend_path(SOLSTICE_FILES_INC ${SOLSTICE_SOURCE_DIR})
rcmake_prepend_path(SOLSTICE_FILES_DOC ${PROJECT_SOURCE_DIR}/../)

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

if(MSVC)
  set(GETOPT_LIB MuslGetopt)
endif()

add_library(sollib STATIC ${SOLSTICE_FILES_SRC} ${SOLSTICE_FILES_INC})
add_executable(solstice ${SOLSTICE_SOURCE_DIR}/main.c)
target_link_libraries(solstice ${MATH_LIB} ${GETOPT_LIB}
  LibYAML RSys sollib solparser SolAnim SolSolver srcvl Star3DUT StarSP StarSTL)
set_target_properties(solstice PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION_MAJOR})
rcmake_copy_runtime_libraries(solstice)

################################################################################
# Tests
################################################################################
if(NOT NO_TEST)
  function(build_test _name)
    add_executable(${_name}
      ${SOLSTICE_SOURCE_DIR}/${_name}.c
      ${SOLSTICE_SOURCE_DIR}/test_solstice_utils.h)
    target_link_libraries(${_name} ${MATH_LIB} ${GETOPT_LIB} RSys sollib)
  endfunction()

  function(new_test _name)
    build_test(${_name})
    add_test(${_name} ${_name})
  endfunction()

  new_test(test_solstice_args)

  build_test(test_solstice_simulation)
  function(add_test_simulation _name)
    add_test(NAME test_solstice_simulation_${_name}
      COMMAND test_solstice_simulation
        $<TARGET_FILE:solstice>
        ${SOLSTICE_SOURCE_DIR}/../yaml/
        ${_name})
  endfunction()

  add_test_simulation(beam_down)
  add_test_simulation(test01)
  add_test_simulation(test02)
  add_test_simulation(test03)
  add_test_simulation(test04)
  add_test_simulation(test05)
  add_test_simulation(test06)
  add_test_simulation(test07)
  add_test_simulation(test08)

endif()

################################################################################
# Define output & install directories
################################################################################
install(TARGETS solstice
  ARCHIVE DESTINATION bin
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(FILES ${SOLSTICE_FILES_DOC} DESTINATION share/doc/solstice)

