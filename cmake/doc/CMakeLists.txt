# Copyright (C) 2016-2018 CNRS, 2018-2019 |Meso|Star>
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

string(REGEX MATCH ".*HTML.*" _html ${SOLSTICE_DOC})
string(REGEX MATCH ".*ROFF.*" _roff ${SOLSTICE_DOC})

set(SOLSTICE_DOC_DIR ${PROJECT_SOURCE_DIR}/../doc)

################################################################################
# Look for asciidoc and a2x programs
################################################################################
if(_html)
  find_program(ASCIIDOC NAMES asciidoc asciidoc.py)
  if(NOT ASCIIDOC)
    unset(_html)
    message(WARNING
      "The `asciidoc' program is missing. "
      "The solstice HTML documentation cannot be generated.")
  endif()
endif()

if(_roff)
  find_program(A2X NAMES a2x a2x.py)
  if(NOT A2X)
    unset(_roff)
    message(WARNING
      "The `a2x' program is missing. "
      "The solstice man pages cannot be generated.")
  endif()
endif()

################################################################################
# Copy doc files
################################################################################
set(MAN_NAMES
  solstice-input.5
  solstice-output.5
  solstice-receiver.5)

if(_roff OR _html)
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
# ROFF man pages
################################################################################
if(_roff)
  set(A2X_OPTS -dmanpage -fmanpage)
  set(MAN_FILES)
  set(MAN5_FILES)
  set(MAN1_FILES)
  foreach(_name IN LISTS MAN_NAMES)
    set(_man ${CMAKE_CURRENT_BINARY_DIR}/${_name})
    set(_txt ${CMAKE_CURRENT_BINARY_DIR}/${_name}.txt)

    add_custom_command(
      OUTPUT ${_man}
      COMMAND ${A2X} ${A2X_OPTS} ${_txt}
      DEPENDS man-copy ${_txt}
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Build ROFF man page ${_man}"
      VERBATIM)
    list(APPEND MAN_FILES ${_man})

    string(REGEX MATCH "^.*.5$" _man5 ${_man})
    string(REGEX MATCH "^.*.1$" _man1 ${_man})
    if(_man1)
      list(APPEND MAN1_FILES ${_man1})
    elseif(_man5)
      list(APPEND MAN5_FILES ${_man5})
    else()
      message(FATAL_ERROR "Unexpected man type")
    endif()
  endforeach()
  add_custom_target(man-roff ALL DEPENDS ${MAN_FILES})

  install(FILES ${MAN1_FILES} DESTINATION share/man/man1)
  install(FILES ${MAN5_FILES} DESTINATION share/man/man5)
endif()

################################################################################
# HTML documentation
################################################################################
if(_html)
  set(ASCIIDOC_OPTS
    -bxhtml11
    -dmanpage
    --attribute themedir=${SOLSTICE_DOC_DIR}
    --theme=solstice-man)

  set(MAN_FILES)
  set(MAN5_FILES)
  set(MAN1_FILES)
  foreach(_name IN LISTS MAN_NAMES)
    set(_man ${CMAKE_CURRENT_BINARY_DIR}/${_name}.html)
    set(_txt ${CMAKE_CURRENT_BINARY_DIR}/${_name}.txt)

    add_custom_command(
      OUTPUT ${_man}
      COMMAND ${ASCIIDOC} ${ASCIIDOC_OPTS} ${_txt}
      DEPENDS man-copy ${_txt} ${SOLSTICE_DOC_DIR}/solstice-man.css
      WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Build HTML man page ${_man}"
      VERBATIM)
    list(APPEND MAN_FILES ${_man})

    string(REGEX MATCH "^.*.5.html$" _man5 ${_man})
    string(REGEX MATCH "^.*.1.html$" _man1 ${_man})
    if(_man1)
      list(APPEND MAN1_FILES ${_man1})
    elseif(_man5)
      list(APPEND MAN5_FILES ${_man5})
    else()
      message(FATAL_ERROR "Unexpected man type")
    endif()
  endforeach()
  add_custom_target(man-html ALL DEPENDS ${MAN_FILES})

  install(FILES ${MAN1_FILES} DESTINATION share/doc/solstice/html)
  install(FILES ${MAN5_FILES} DESTINATION share/doc/solstice/html)
endif()

