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
# Findprotobuf-c
# -------------
#
# Finds the protobuf-c library
#
# This wil define the following variables::
#
# PROTOBUF_C_FOUND      - True if the system has the protobuf-c library
# PROTOBUF_C_VERSION    - The version of the protobuf-c library which was found
# PROTOC_C_EXECUTABLE   - The protoc-c executable, protoc-c-NOTFOUND otherwise
# PROTOC_GEN_C_EXECUTABLE
#
# and the following imported targets::
#
# protobuf-c::protobuf-c    - The protobuf-c library
# protobuf-c::protoc-c      - The protoc-c utility executable
# protobuf-c::protoc-gen-c  - The protoc gen-c plugin utility executable (1.3 only)
# protobuf:protoc           - The protoc command (for compatibility with other CMake scripts)
#
# Based on https://cmake.org/cmake/help/v3.10/manual/cmake-developer.7.html#a-sample-find-module

find_package(PkgConfig)
pkg_check_modules(PC_PROTOBUF_C protobuf-c QUIET)

find_path(PROTOBUF_C_INCLUDE_DIR
  NAMES protobuf-c/protobuf-c.h
  PATHS ${PC_PROTOBUF_C_INCLUDE_DIRS})
find_library(PROTOBUF_C_LIBRARY
  NAMES protobuf-c
  PATHS ${PC_PROTOBUF_C_LIBRARY_DIRS})

set(PROTOBUF_C_VERSION ${PC_PROTOBUF_C_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(PROTOBUF_C
  FOUND_VAR PROTOBUF_C_FOUND
  REQUIRED_VARS PROTOBUF_C_LIBRARY PROTOBUF_C_INCLUDE_DIR
  VERSION_VAR PROTOBUF_C_VERSION)

if(PROTOBUF_C_FOUND)
  set(PROTOBUF_C_LIBRARIES ${PROTOBUF_C_LIBRARY})
  set(PROTOBUF_C_INCLUDE_DIRS ${PROTOBUF_C_INCLUDE_DIR})
  set(PROTOBUF_C_DEFINITIONS ${PC_PROTOBUF_C_CFLAGS_OTHER})
endif()

if(PROTOBUF_C_FOUND AND NOT TARGET protobuf-c::protobuf-c)
  add_library(protobuf-c::protobuf-c UNKNOWN IMPORTED)
  set_target_properties(protobuf-c::protobuf-c PROPERTIES
    IMPORTED_LOCATION "${PROTOBUF_C_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_PROTOBUF_C_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_C_INCLUDE_DIR}")
endif()

find_program(PROTOC_C_EXECUTABLE NAMES protoc-c)

if (PROTOC_C_EXECUTABLE AND NOT TARGET protobuf-c::protoc-c)
  add_executable(protobuf-c::protoc-c IMPORTED)
  set_target_properties(protobuf-c::protoc-c PROPERTIES
    IMPORTED_LOCATION "${PROTOC_C_EXECUTABLE}")
endif()

find_program(PROTOC_EXECUTABLE NAMES protoc)

if (PROTOC_EXECUTABLE AND NOT TARGET protobuf::protoc)
  add_executable(protobuf::protoc IMPORTED)
  set_target_properties(protobuf::protoc PROPERTIES
    IMPORTED_LOCATION "${PROTOC_EXECUTABLE}")
endif()

find_program(PROTOC_GEN_C_EXECUTABLE NAMES protoc-gen-c)

# 1.3 only
if (PROTOC_GEN_C_EXECUTABLE AND NOT TARGET protobuf-c::protoc-gen-c)
  add_executable(protobuf-c::protoc-gen-c IMPORTED)
  set_target_properties(protobuf-c::protoc-gen-c PROPERTIES
    IMPORTED_LOCATION "${PROTOC_GEN_C_EXECUTABLE}")
endif()
