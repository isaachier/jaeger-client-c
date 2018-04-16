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

cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
    CACHE FILEPATH
    "Toolchain to use for building this package and dependencies")

# TODO: Get rid of this when not debugging
set(HUNTER_KEEP_PACKAGE_SOURCES ON)

include(HunterGate)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.20.35.tar.gz"
    SHA1 "6e3cb4c333b76803a83c56fcbca3b0bfd9f96f27"
    LOCAL
)

project(jaegertracingc VERSION 0.0.1)

include(CheckCCompilerFlag)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CMakeDependentOption)
include(CTest)

include(CheckAtomics)
include(CheckAttributes)
include(GenerateDocumentation)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if(CMAKE_C_COMPILER_ID STREQUAL "GNU" AND
   CMAKE_C_COMPILER_VERSION VERSION_GREATER_EQUAL "5.0")
 set(advanced_flags "-Wextra")
endif()

foreach(flag "-Wall" "-Werror" ${advanced_flags})
  string(MAKE_C_IDENTIFIER "HAVE${flag}" flag_var)
  check_c_compiler_flag(${flag} ${flag_var})
  if(${flag_var})
    list(APPEND jaegertracingc_flags ${flag})
  endif()
endforeach()

if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(debug_build ON)
endif()
cmake_dependent_option(JAEGERTRACINGC_COVERAGE "Enable code coverage" OFF
                       "BUILD_TESTING;debug_build" OFF)
if(JAEGERTRACINGC_COVERAGE)
  include(CodeCoverage)
  append_coverage_compiler_flags(CMAKE_C_FLAGS)
  set(COVERAGE_EXCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test_driver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test_driver.h"
    "${CMAKE_CURRENT_BINARY_DIR}/src/jaegertracingc/protoc-gen/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src/*")
endif()

cmake_dependent_option(JAEGERTRACINGC_VERBOSE_ALLOC "Print all allocations" OFF
                       "BUILD_TESTING;debug_build" OFF)
mark_as_advanced(JAEGERTRACINGC_VERBOSE_ALLOC)

hunter_add_package(opentracing-c)
find_package(opentracing-c CONFIG REQUIRED)

hunter_add_package(http-parser)
find_package(http-parser CONFIG REQUIRED)

hunter_add_package(jansson)
find_package(jansson CONFIG REQUIRED)

hunter_add_package(Protobuf)
find_package(Protobuf CONFIG REQUIRED)

hunter_add_package(protobuf-c)
find_package(protobuf-c CONFIG REQUIRED)

hunter_add_package(pcg)
find_package(pcg CONFIG REQUIRED)

option(JAEGERTRACINGC_MT "Enable multithreading support" ON)
if(JAEGERTRACINGC_MT)
  set(CMAKE_THREAD_PREFER_PTHREAD ON)
  find_package(Threads REQUIRED)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(core_src
  "src/jaegertracingc/alloc.c"
  "src/jaegertracingc/alloc.h"
  "src/jaegertracingc/clock.c"
  "src/jaegertracingc/clock.h"
  "src/jaegertracingc/common.c"
  "src/jaegertracingc/common.h"
  "src/jaegertracingc/constants.c"
  "src/jaegertracingc/key_value.c"
  "src/jaegertracingc/key_value.h"
  "src/jaegertracingc/log_record.c"
  "src/jaegertracingc/log_record.h"
  "src/jaegertracingc/logging.c"
  "src/jaegertracingc/logging.h"
  "src/jaegertracingc/metrics.c"
  "src/jaegertracingc/metrics.h"
  "src/jaegertracingc/net.c"
  "src/jaegertracingc/net.h"
  "src/jaegertracingc/random.c"
  "src/jaegertracingc/random.h"
  "src/jaegertracingc/reporter.c"
  "src/jaegertracingc/reporter.h"
  "src/jaegertracingc/sampler.c"
  "src/jaegertracingc/sampler.h"
  "src/jaegertracingc/sampling_strategy.c"
  "src/jaegertracingc/sampling_strategy.h"
  "src/jaegertracingc/span.c"
  "src/jaegertracingc/span.h"
  "src/jaegertracingc/tag.c"
  "src/jaegertracingc/tag.h"
  "src/jaegertracingc/threading.c"
  "src/jaegertracingc/threading.h"
  "src/jaegertracingc/token_bucket.c"
  "src/jaegertracingc/token_bucket.h"
  "src/jaegertracingc/trace_id.c"
  "src/jaegertracingc/trace_id.h"
  "src/jaegertracingc/tracer.c"
  "src/jaegertracingc/tracer.h"
  "src/jaegertracingc/vector.c"
  "src/jaegertracingc/vector.h")

set(generated_src_dir "${CMAKE_CURRENT_BINARY_DIR}/src/jaegertracingc")

file(MAKE_DIRECTORY "${generated_src_dir}/protoc-gen")
function(generate_protobuf_c gen_src)
  foreach(proto_src ${ARGN})
    get_filename_component(component ${proto_src} NAME_WE)
    get_filename_component(dir ${proto_src} DIRECTORY)
    set(protoc_out_files
        "${generated_src_dir}/protoc-gen/${component}.pb-c.h"
        "${generated_src_dir}/protoc-gen/${component}.pb-c.c")
    add_custom_command(
      OUTPUT ${protoc_out_files}
      COMMAND protobuf::protoc
      ARGS --plugin=$<TARGET_FILE:protobuf-c::protoc-gen-c>
           --c_out "${generated_src_dir}/protoc-gen"
           -I. "${component}.proto"
      MAIN_DEPENDENCY ${proto_src}
      WORKING_DIRECTORY ${dir}
      VERBATIM
      USES_TERMINAL)
    list(APPEND out_files ${protoc_out_files})
  endforeach()
  set(${gen_src} ${out_files} PARENT_SCOPE)
endfunction()

generate_protobuf_c(
  protoc_gen_src
  "${CMAKE_CURRENT_SOURCE_DIR}/idl/protobuf/agent.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/idl/protobuf/baggage.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/idl/protobuf/jaeger.proto"
  "${CMAKE_CURRENT_SOURCE_DIR}/idl/protobuf/sampling.proto")

add_library(jaegertracingc ${core_src} ${protoc_gen_src})
target_compile_options(jaegertracingc PRIVATE ${jaegertracingc_flags})

configure_file(
  "src/jaegertracingc/constants.h.in"
  "${generated_src_dir}/constants.h"
  @ONLY)
configure_file(
  "src/jaegertracingc/config.h.in"
  "${generated_src_dir}/config.h"
  @ONLY)
target_include_directories(jaegertracingc PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/src>
  $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/src>
  $<INSTALL_INTERFACE:include>)

target_link_libraries(jaegertracingc PUBLIC
	pcg::pcg_random
  http-parser::http_parser
  jansson::jansson
  protobuf-c::protobuf-c)
if(JAEGERTRACINGC_MT)
  target_link_libraries(jaegertracingc PUBLIC Threads::Threads)
  target_compile_definitions(jaegertracingc PUBLIC JAEGERTRACINGC_MT)
endif()

check_atomics(have_atomics)
if(have_atomics)
  target_compile_definitions(jaegertracingc PUBLIC
    "-DJAEGERTRACINGC_HAVE_ATOMICS")
else()
endif()

check_symbol_exists(
  "SYS_getrandom" "linux/random.h;sys/syscall.h;unistd.h" have_getrandom)
if(have_getrandom)
  target_compile_definitions(jaegertracingc PRIVATE "-DHAVE_GETRANDOM")
else()
  check_symbol_exists("arc4random_buf" "stdlib.h" have_arc4random_buf)
  if(have_arc4random_buf)
    target_compile_definitions(jaegertracingc PRIVATE "-DHAVE_ARC4RANDOM_BUF")
  endif()
endif()

check_attributes()

if(JAEGERTRACINGC_VERBOSE_ALLOC)
  target_compile_definitions(
    jaegertracingc PUBLIC "-DJAEGERTRACINGC_VERBOSE_ALLOC")
endif()

if(BUILD_TESTING)
  add_library(unity "third_party/unity/src/unity.c")
  target_include_directories(unity PUBLIC "third_party/unity/src")

  set(test_src
    "src/jaegertracingc/alloc_test.c"
    "src/jaegertracingc/clock_test.c"
    "src/jaegertracingc/logging_test.c"
    "src/jaegertracingc/metrics_test.c"
    "src/jaegertracingc/net_test.c"
    "src/jaegertracingc/random_test.c"
    "src/jaegertracingc/reporter_test.c"
    "src/jaegertracingc/sampler_test.c"
    "src/jaegertracingc/span_test.c"
    "src/jaegertracingc/tag_test.c"
    "src/jaegertracingc/threading_test.c"
    "src/jaegertracingc/trace_id_test.c"
    "src/jaegertracingc/token_bucket_test.c"
    "src/jaegertracingc/vector_test.c")
  foreach(test_file ${test_src})
    get_filename_component(test_component ${test_file} NAME_WE)
    string(REPLACE "_test" "" test_component "${test_component}")
    string(TOUPPER "${test_component}" test_component_upper)
    configure_file(
      "src/jaegertracingc/test.h.in"
      "${generated_src_dir}/${test_component}_test.h"
      @ONLY)

    set(unit_test_driver_includes
        "${unit_test_driver_includes}\n#include \"${test_component}_test.h\"")
    set(unit_test_driver_impl
        "${unit_test_driver_impl}\n    RUN_TEST(test_${test_component});")
  endforeach()
  configure_file(
    "src/jaegertracingc/unit_test_driver.h.in"
    "${generated_src_dir}/unit_test_driver.h"
    @ONLY)
  add_library(jaegertracingc_test ${test_src} "src/jaegertracingc/mock_agent.c")
  target_include_directories(jaegertracingc_test PUBLIC
    "src/jaegertracingc" $<BUILD_INTERFACE:${generated_src_dir}>)
  target_link_libraries(jaegertracingc_test PUBLIC jaegertracingc unity)
  target_compile_definitions(jaegertracingc_test PUBLIC
    UNITY_USE_COMMAND_LINE_ARGS)

	foreach(driver default)
	  add_executable(${driver}_test "src/jaegertracingc/${driver}_test_driver.c")
	  target_link_libraries(${driver}_test PUBLIC jaegertracingc_test)
  	add_test(${driver}_test ${driver}_test)
	endforeach()

  if(JAEGERTRACINGC_COVERAGE)
    setup_target_for_coverage(
      NAME unit_test_coverage
      EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/default_test
      DEPENDENCIES default_test)
  endif()
endif()

generate_documentation()
