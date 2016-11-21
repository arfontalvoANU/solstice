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
project(solstice C CXX)
enable_testing()
option(NO_TEST "Do not build tests" OFF)

set(SOLSTICE_SOURCE_DIR ${PROJECT_SOURCE_DIR}/../src/)

################################################################################
# Check dependencies
################################################################################
get_filename_component(_current_source_dir ${CMAKE_CURRENT_LIST_FILE} PATH)
set(LibYAML_DIR ${_current_source_dir}/)

find_package(LibYAML REQUIRED)
find_package(RCMake 0.2.3 REQUIRED)
find_package(RSys 0.3 REQUIRED)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${RCMAKE_SOURCE_DIR})
include(rcmake)
include(rcmake_runtime)

include_directories(${RSys_INCLUDE_DIR})

################################################################################
# Build subprojects
################################################################################
add_subdirectory(parser)

################################################################################
# Configure and define targets
################################################################################
set(VERSION_MAJOR 0)
set(VERSION_MINOR 1)
set(VERSION_PATCH 0)
set(VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH})

set(SOLSTICE_FILES_SRC solstice.c)
set(SOLSTICE_FILES_INC )
set(SOLSTICE_FILES_DOC COPYING README.md)

# Prepend each file in the `SOLSTICE_FILES_<SRC|INC>' list by `SOLSTICE_SOURCE_DIR'
rcmake_prepend_path(SOLSTICE_FILES_SRC ${SOLSTICE_SOURCE_DIR})
rcmake_prepend_path(SOLSTICE_FILES_INC ${SOLSTICE_SOURCE_DIR})
rcmake_prepend_path(SOLSTICE_FILES_DOC ${PROJECT_SOURCE_DIR}/../)

if(CMAKE_COMPILER_IS_GNUCC)
  set(MATH_LIB m)
endif()

add_executable(solstice ${SOLSTICE_SOURCE_DIR}/solstice.c)
target_link_libraries(solstice LibYAML RSys ${MATH_LIB} solstice-parser)
set_target_properties(solstice PROPERTIES
  VERSION ${VERSION}
  SOVERSION ${VERSION_MAJOR})
rcmake_copy_runtime_libraries(solstice)

################################################################################
# Define output & install directories
################################################################################
install(TARGETS solstice
  ARCHIVE DESTINATION bin
  LIBRARY DESTINATION lib
  RUNTIME DESTINATION bin)
install(FILES ${SOLSTICE_FILES_DOC} DESTINATION share/doc/solstice)

