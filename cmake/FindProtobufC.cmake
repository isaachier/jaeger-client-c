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
# FindProtobufC
# -------------
#
# Finds the protobuf-c library
#
# This wil define the following variables::
#
# PROTOBUF_C_FOUND      - True if the system has the ProtobufC library
# PROTOBUF_C_VERSION    - The version of the ProtobufC library which was found
# PROTOC_C_EXECUTABLE   - The protoc-c executable, protoc-c-NOTFOUND otherwise
#
# and the following imported targets::
#
# ProtobufC::ProtobufC    - The ProtobufC library
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

if(PROTOBUF_C_FOUND AND NOT TARGET ProtobufC::ProtobufC)
  add_library(ProtobufC::ProtobufC UNKNOWN IMPORTED)
  set_target_properties(ProtobufC::ProtobufC PROPERTIES
    IMPORTED_LOCATION "${PROTOBUF_C_LIBRARY}"
    INTERFACE_COMPILE_OPTIONS "${PC_PROTOBUF_C_CFLAGS_OTHER}"
    INTERFACE_INCLUDE_DIRECTORIES "${PROTOBUF_C_INCLUDE_DIR}")
endif()

find_program(PROTOC_C_EXECUTABLE NAMES protoc-c)
