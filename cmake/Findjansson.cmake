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
# FindJansson
# -------------
#
# Finds the Jansson library
#
# This wil define the following variables::
#
# JANSSON_FOUND      - True if the system has the Jansson library
# JANSSON_VERSION    - The version of the Jansson library which was found
#
# and the following imported targets::
#
# jansson::jansson    - The Jansson library
#
# Based on https://cmake.org/cmake/help/v3.10/manual/cmake-developer.7.html#a-sample-find-module

find_package(PkgConfig)
pkg_check_modules(PC_JANSSON QUIET jansson)

find_path(JANSSON_INCLUDE_DIR
  NAMES jansson.h
  PATHS ${PC_JANSSON_INCLUDE_DIRS}
  PATH_SUFFIXES jansson)
find_library(JANSSON_LIBRARY
  NAMES jansson
  PATHS ${PC_JANSSON_LIBRARY_DIRS})

set(JANSSON_VERSION ${PC_JANSSON_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(JANSSON
  FOUND_VAR JANSSON_FOUND
  REQUIRED_VARS JANSSON_LIBRARY JANSSON_INCLUDE_DIR
  VERSION_VAR JANSSON_VERSION)

if(JANSSON_FOUND)
  set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
  set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
  set(JANSSON_DEFINITIONS ${PC_JANSSON_CFLAGS_OTHER})
endif()

if(JANSSON_FOUND AND NOT TARGET jansson::jansson)
  add_library(jansson::jansson UNKNOWN IMPORTED)
  set_target_properties(jansson::jansson PROPERTIES
    IMPORTED_LOCATION "${JANSSON_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_JANSSON_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${JANSSON_INCLUDE_DIR}")
endif()
