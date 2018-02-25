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

function(generate_documentation)
  set(copyright_holder "Uber Technologies, Inc.")
  string(TIMESTAMP copyright_year "%Y")
  set(sphinx_relative_path "doc")
  set(sphinx_src_path "${CMAKE_CURRENT_SOURCE_DIR}/${sphinx_relative_path}")
  set(sphinx_out_path "${CMAKE_CURRENT_BINARY_DIR}/${sphinx_relative_path}")
  set(static_path "${sphinx_src_path}/_static")
  set(templates_path "${sphinx_src_path}/_templates")
  file(RELATIVE_PATH constants_in_h_rel_path
    ${sphinx_out_path}
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/constants.h.in")
  get_filename_component(src_rel_path
    ${constants_in_h_rel_path}
    DIRECTORY)
  file(RELATIVE_PATH constants_gen_h_rel_path
    ${sphinx_out_path}
    "${CMAKE_CURRENT_BINARY_DIR}/include/jaegertracingc/constants.h")
  get_filename_component(gen_inc_rel_path
    ${constants_gen_h_rel_path}
    DIRECTORY)

  get_target_property(defs jaegertracingc COMPILE_DEFINITIONS)

  configure_file("${sphinx_src_path}/conf.py.in"
                 "${sphinx_out_path}/conf.py" @ONLY)
  configure_file("${sphinx_src_path}/index.rst.in"
                 "${sphinx_out_path}/index.rst" @ONLY)
  add_custom_target(doc
    COMMAND ${sphinx_build} -M html ${sphinx_out_path} ${sphinx_out_path}
    VERBATIM
    USES_TERMINAL)
endfunction()
