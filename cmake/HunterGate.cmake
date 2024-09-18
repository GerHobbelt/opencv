# Copyright (c) 2013-2015, Ruslan Baratov
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
#   this list of conditions and the following disclaimer in the documentation
#   and/or other materials provided with the distribution.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

# This is a gate file to Hunter package manager.
# Include this file using `include` command and add package you need, example:
#
#     cmake_minimum_required(VERSION 3.5)
#
#     include("cmake/HunterGate.cmake")
#     HunterGate(
#         URL "https://github.com/path/to/hunter/archive.tar.gz"
#         SHA1 "798501e983f14b28b10cda16afa4de69eee1da1d"
#     )
#
#     project(MyProject)
#
#     hunter_add_package(Foo)
#     hunter_add_package(Boo COMPONENTS Bar Baz)
#
# Projects:
#     * https://github.com/hunter-packages/gate/
#     * https://github.com/ruslo/hunter

cmake_minimum_required(VERSION 3.10) # Minimum for Hunter
include(CMakeParseArguments) # cmake_parse_arguments

option(HUNTER_ENABLED "Enable Hunter package manager support" ON)
option(HUNTER_STATUS_PRINT "Print working status" ON)
option(HUNTER_STATUS_DEBUG "Print a lot info" OFF)

set(HUNTER_WIKI "https://github.com/ruslo/hunter/wiki")

function(hunter_gate_wiki wiki_page)
  message("------------------------------ WIKI -------------------------------")
  message("    ${HUNTER_WIKI}/${wiki_page}")
  message("-------------------------------------------------------------------")
  message("")
  message(FATAL_ERROR "")
endfunction()

function(hunter_gate_internal_error)
  message("")
  foreach(print_message ${ARGV})
    message("[hunter ** INTERNAL **] ${print_message}")
  endforeach()
  message("[hunter ** INTERNAL **] [Directory:${CMAKE_CURRENT_LIST_DIR}]")
  message("")
  hunter_gate_wiki("error.internal")
endfunction()

function(hunter_gate_fatal_error)
  cmake_parse_arguments(hunter "" "WIKI" "" "${ARGV}")
  if(NOT hunter_WIKI)
    hunter_gate_internal_error("Expected wiki")
  endif()
  message("")
  foreach(x ${hunter_UNPARSED_ARGUMENTS})
    message("[hunter ** FATAL ERROR **] ${x}")
  endforeach()
  message("[hunter ** FATAL ERROR **] [Directory:${CMAKE_CURRENT_LIST_DIR}]")
  message("")
  hunter_gate_wiki("${hunter_WIKI}")
endfunction()

function(hunter_gate_user_error)
  hunter_gate_fatal_error(${ARGV} WIKI "error.incorrect.input.data")
endfunction()

function(hunter_gate_self root version sha1 result)
  string(COMPARE EQUAL "${root}" "" is_bad)
  if(is_bad)
    hunter_gate_internal_error("root is empty")
  endif()

  string(COMPARE EQUAL "${version}" "" is_bad)
  if(is_bad)
    hunter_gate_internal_error("version is empty")
  endif()

  string(COMPARE EQUAL "${sha1}" "" is_bad)
  if(is_bad)
    hunter_gate_internal_error("sha1 is empty")
  endif()

  string(SUBSTRING "${sha1}" 0 7 archive_id)

  if(EXISTS "${root}/cmake/Hunter")
    set(hunter_self "${root}")
  else()
    set(
        hunter_self
        "${root}/_Base/Download/Hunter/${version}/${archive_id}/Unpacked"
    )
  endif()

  set("${result}" "${hunter_self}" PARENT_SCOPE)
endfunction()

# Set HUNTER_GATE_ROOT cmake variable to suitable value.
function(hunter_gate_detect_root)
  # Check CMake variable
  if(HUNTER_ROOT)
    set(HUNTER_GATE_ROOT "${HUNTER_ROOT}" PARENT_SCOPE)
    return()
  endif()

  # Check environment variable
  string(COMPARE NOTEQUAL "$ENV{HUNTER_ROOT}" "" not_empty)
  if(not_empty)
    set(HUNTER_GATE_ROOT "$ENV{HUNTER_ROOT}" PARENT_SCOPE)
    return()
  endif()

  hunter_gate_fatal_error(
      "Can't detect HUNTER_ROOT"
      WIKI "error.detect.hunter.root"
  )
endfunction()

# Must be a macro so master file 'cmake/Hunter' can
# apply all variables easily just by 'include' command
# (otherwise PARENT_SCOPE magic needed)
macro(HunterGate)
  if(HUNTER_GATE_DONE)
    # variable HUNTER_GATE_DONE set explicitly for external project
    # (see `hunter_download`)
    set_property(GLOBAL PROPERTY HUNTER_GATE_DONE YES)
  endif()

  # First HunterGate command will init Hunter, others will be ignored
  get_property(_hunter_gate_done GLOBAL PROPERTY HUNTER_GATE_DONE SET)

  if(_hunter_gate_done)
    hunter_gate_self(
        "${HUNTER_CACHED_ROOT}"
        "${HUNTER_VERSION}"
        "${HUNTER_SHA1}"
        _hunter_self
    )
    include("${_hunter_self}/cmake/Hunter")
  else()
    set(HUNTER_GATE_LOCATION "${CMAKE_CURRENT_LIST_DIR}")

    if(PROJECT_NAME)

    message("----------${PROJECT_NAME}")
      hunter_gate_fatal_error(
          "Please set HunterGate *before* 'project' command"
          WIKI "error.huntergate.before.project"
      )
    endif()

    cmake_parse_arguments(
        HUNTER_GATE "LOCAL" "URL;SHA1;GLOBAL;FILEPATH" "" ${ARGV}
    )
    if(HUNTER_GATE_UNPARSED_ARGUMENTS)
      hunter_gate_user_error(
          "HunterGate unparsed arguments: ${HUNTER_GATE_UNPARSED_ARGUMENTS}"
      )
    endif()
    if(HUNTER_GATE_GLOBAL)
      if(HUNTER_GATE_LOCAL)
        hunter_gate_user_error("Unexpected LOCAL (already has GLOBAL)")
      endif()
      if(HUNTER_GATE_FILEPATH)
        hunter_gate_user_error("Unexpected FILEPATH (already has GLOBAL)")
      endif()
    endif()
    if(HUNTER_GATE_LOCAL)
      if(HUNTER_GATE_GLOBAL)
        hunter_gate_user_error("Unexpected GLOBAL (already has LOCAL)")
      endif()
      if(HUNTER_GATE_FILEPATH)
        hunter_gate_user_error("Unexpected FILEPATH (already has LOCAL)")
      endif()
    endif()
    if(HUNTER_GATE_FILEPATH)
      if(HUNTER_GATE_GLOBAL)
        hunter_gate_user_error("Unexpected GLOBAL (already has FILEPATH)")
      endif()
      if(HUNTER_GATE_LOCAL)
        hunter_gate_user_error("Unexpected LOCAL (already has FILEPATH)")
      endif()
    endif()

    hunter_gate_detect_root() # set HUNTER_GATE_ROOT

    # Beautify path, fix probable problems with windows path slashes
    get_filename_component(
        HUNTER_GATE_ROOT "${HUNTER_GATE_ROOT}" ABSOLUTE
    )
    string(FIND "${HUNTER_GATE_ROOT}" " " _contain_spaces)
    if(NOT _contain_spaces EQUAL -1)
      hunter_gate_fatal_error(
          "HUNTER_ROOT (${HUNTER_GATE_ROOT}) contains spaces"
          WIKI "error.spaces.in.hunter.root"
      )
    endif()

    string(
        REGEX
        MATCH
        "[0-9]+\\.[0-9]+\\.[0-9]+[-_a-z0-9]*"
        HUNTER_GATE_VERSION
        "${HUNTER_GATE_URL}"
    )
    string(COMPARE EQUAL "${HUNTER_GATE_VERSION}" "" _is_empty)
    if(_is_empty)
      set(HUNTER_GATE_VERSION "unknown")
    endif()

    hunter_gate_self(
        "${HUNTER_GATE_ROOT}"
        "${HUNTER_GATE_VERSION}"
        "${HUNTER_GATE_SHA1}"
        hunter_self_
    )

    set(_master_location "${hunter_self_}/cmake/Hunter")
    if(EXISTS "${HUNTER_GATE_ROOT}/cmake/Hunter")
      # Hunter downloaded manually (e.g. by 'git clone')
      set(_unused "xxxxxxxxxx")
      set(HUNTER_GATE_SHA1 "${_unused}")
      set(HUNTER_GATE_VERSION "${_unused}")
    else()
      hunter_gate_user_error(
          "Master file not found:"
          "  ${_master_location}"
          "try to update Hunter/HunterGate"
      )
    endif()
    include("${_master_location}")
    set_property(GLOBAL PROPERTY HUNTER_GATE_DONE YES)
  endif()
endmacro()
