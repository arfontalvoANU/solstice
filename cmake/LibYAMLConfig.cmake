# Copyright (C) 2015-2016 CNRS
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

# Try to find the LibYAML devel. Once done this will define:
#   - LibYAML_FOUND: system has LibYAML
#   - LibYAML_INCLUDE_DIR: the include directory
#   - LibYAML: Link this to use LibYAML

find_path(LibYAML_INCLUDE_DIR yaml.h)
unset(LibYAML_LIBRARY CACHE)
unset(LibYAML_LIBRARY_DEBUG CACHE)
unset(LibYAML_LIBRARY_RELWITHDEBINFO CACHE)
unset(LibYAML_LIBRARY_MINSIZEREL CACHE)
find_library(LibYAML_LIBRARY yaml DOC
  "Path to the LibYAML library used during release builds."
  PATH_SUFFIXES bin)
find_library(LibYAML_LIBRARY_DEBUG yaml-dbg DOC
  "Path to the LibYAML library used during debug builds."
  PATH_SUFFIXES bin)
if(NOT LibYAML_LIBRARY_DEBUG)
  set(LibYAML_LIBRARY_DEBUG ${LibYAML_LIBRARY})
endif()

# Create the imported library target
if(CMAKE_HOST_WIN32)
  set(_property IMPORTED_IMPLIB)
else(CMAKE_HOST_WIN32)
  set(_property IMPORTED_LOCATION)
endif(CMAKE_HOST_WIN32)
add_library(LibYAML SHARED IMPORTED)
set_target_properties(LibYAML PROPERTIES
  ${_property} ${LibYAML_LIBRARY_DEBUG}
  ${_property}_DEBUG ${LibYAML_LIBRARY_DEBUG}
  ${_property}_RELEASE ${LibYAML_LIBRARY})

# Check the package
include(FindPackageHandleStandardArgs)
if(CMAKE_HOST_WIN32)
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibYAML DEFAULT_MSG
    LibYAML_INCLUDE_DIR
    LibYAML_LIBRARY_DEBUG
    LibYAML_LIBRARY)
else()
  FIND_PACKAGE_HANDLE_STANDARD_ARGS(LibYAML DEFAULT_MSG
    LibYAML_INCLUDE_DIR
    LibYAML_LIBRARY)
endif()

