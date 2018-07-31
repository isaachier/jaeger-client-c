#
# Copyright (c) 2018 Uber Technologies, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# .rst
# Findhttp-parser
# -------------
#
# Finds the http-parser library
#
# This wil define the following variables::
#
# HTTP_PARSER_FOUND      - True if the system has the http-parser library
# HTTP_PARSER_VERSION    - The version of the http-parser library which was found
#
# and the following imported targets::
#
# http-parser::http_parser    - The http-parser library
#
# Based on https://cmake.org/cmake/help/v3.10/manual/cmake-developer.7.html#a-sample-find-module

find_package(PkgConfig)
pkg_check_modules(PC_HTTP_PARSER QUIET http-parser)

find_path(HTTP_PARSER_INCLUDE_DIR
  NAMES http_parser.h
  PATHS ${PC_HTTP_PARSER_INCLUDE_DIRS}
  PATH_SUFFIXES http_parser)
find_library(HTTP_PARSER_LIBRARY
  NAMES http_parser httpparser
  PATHS ${PC_HTTP_PARSER_LIBRARY_DIRS})

set(HTTP_PARSER_VERSION ${PC_HTTP_PARSER_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(HTTP_PARSER
  FOUND_VAR HTTP_PARSER_FOUND
  REQUIRED_VARS HTTP_PARSER_LIBRARY HTTP_PARSER_INCLUDE_DIR
  VERSION_VAR HTTP_PARSER_VERSION)

if(HTTP_PARSER_FOUND)
  set(HTTP_PARSER_LIBRARIES ${HTTP_PARSER_LIBRARY})
  set(HTTP_PARSER_INCLUDE_DIRS ${HTTP_PARSER_INCLUDE_DIR})
  set(HTTP_PARSER_DEFINITIONS ${PC_HTTP_PARSER_CFLAGS_OTHER})
endif()

if(HTTP_PARSER_FOUND AND NOT TARGET http-parser::http_parser)
  add_library(http-parser::http_parser UNKNOWN IMPORTED)
  set_target_properties(http-parser::http_parser PROPERTIES
    IMPORTED_LOCATION "${HTTP_PARSER_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_HTTP_PARSER_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${HTTP_PARSER_INCLUDE_DIR}")
endif()
