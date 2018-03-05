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

if(__GENERATE_DOCUMENTATION)
  return()
endif()
set(__GENERATE_DOCUMENTATION ON)

include(CMakeDependentOption)

function(generate_documentation)
  find_package(Doxygen)
  cmake_dependent_option(JAEGERTRACINGC_BUILD_DOC "Build documentation" ON
                         "DOXYGEN_EXECUTABLE" OFF)
  if(NOT JAEGERTRACINGC_BUILD_DOC)
    return()
  endif()

  get_target_property(defs jaegertracingc COMPILE_DEFINITIONS)
  get_target_property(inc jaegertracingc INCLUDE_DIRECTORIES)

  set(doxygen_out_path "${CMAKE_CURRENT_BINARY_DIR}/doc/$<CONFIG>")

  file(READ "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in" doxyfile_content)
  string(CONFIGURE "${doxyfile_content}" doxyfile_content @ONLY)
  file(GENERATE OUTPUT "${doxygen_out_path}/Doxyfile"
       CONTENT "${doxyfile_content}")

  add_custom_target(doc
    COMMAND ${DOXYGEN_EXECUTABLE} "${doxygen_out_path}/Doxyfile"
    DEPENDS "${doxygen_out_path}/Doxyfile"
            "${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
    VERBATIM
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    USES_TERMINAL)
endfunction()
