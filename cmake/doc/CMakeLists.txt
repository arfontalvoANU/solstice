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

################################################################################
# Look for asciidoc and a2x programs
################################################################################
find_program(ASCIIDOC asciidoc)
set(SOLSTICE_DOC_DIR ${PROJECT_SOURCE_DIR}/../doc)

if(NOT ASCIIDOC)
  message(WARNING
    "The `asciidoc' program is missing. "
    "The solstice HTML documentation cannot be generated.")
endif()

if(CMAKE_HOST_UNIX)
  find_program(A2X a2x)
  if(NOT A2X)
    message(WARNING
      "The `a2x' program is missing. "
      "The solstice man pages cannot be generated.")
  endif()
endif()

################################################################################
# Copy doc files
################################################################################
set(MAN_NAMES
  solstice-input.5)

if(A2X OR ASCIIDOC)
  set(MAN_FILES)
  foreach(_name IN LISTS MAN_NAMES)
    set(_src ${SOLSTICE_DOC_DIR}/${_name}.txt)
    set(_dst ${CMAKE_CURRENT_BINARY_DIR}/${_name}.txt)
    add_custom_command(
      OUTPUT ${_dst}
      COMMAND ${CMAKE_COMMAND} -E copy ${_src} ${_dst}
      DEPENDS ${_src}
      COMMENT "Copy the asciidoc ${_src}"
      VERBATIM)
    list(APPEND MAN_FILES ${_dst})
  endforeach()
  add_custom_target(man-copy ALL DEPENDS ${MAN_FILES})
endif()

list(APPEND MAN_NAMES solstice.1)

################################################################################
# Generate the TROFF man pages
################################################################################
if(A2X)
  set(A2X_OPTS -dmanpage -fmanpage)
  set(MAN_FILES)
  foreach(_name IN LISTS MAN_NAMES)
    set(_man ${CMAKE_CURRENT_BINARY_DIR}/${_name})
    set(_txt ${CMAKE_CURRENT_BINARY_DIR}/${_name}.txt)

    add_custom_command(
      OUTPUT ${_man}
      COMMAND ${A2X} ${A2X_OPTS} ${_txt}
      DEPENDS man-copy ${_txt}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Build TROFF man page ${_txt}"
      VERBATIM)
    list(APPEND MAN_FILES ${_man})
  endforeach()
  add_custom_target(man-troff ALL DEPENDS ${MAN_FILES})
endif()

################################################################################
# Generate the HTML documentation
################################################################################
if(ASCIIDOC)
  set(ASCIIDOC_OPTS
    -bxhtml11
    -dmanpage
    --attribute themedir=${SOLSTICE_DOC_DIR}
    --theme=solstice-man)

  set(MAN_FILES)
  foreach(_name IN LISTS MAN_NAMES)
    set(_man ${CMAKE_CURRENT_BINARY_DIR}/${_name}.html)
    set(_txt ${CMAKE_CURRENT_BINARY_DIR}/${_name}.txt)

    add_custom_command(
      OUTPUT ${_man}
      COMMAND ${ASCIIDOC} ${ASCIIDOC_OPTS} ${_txt}
      DEPENDS man-copy ${_txt}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Build HTML man page ${_txt}"
      VERBATIM)
    list(APPEND MAN_FILES ${_man})
  endforeach()
  add_custom_target(man-html ALL DEPENDS ${MAN_FILES})
endif()

