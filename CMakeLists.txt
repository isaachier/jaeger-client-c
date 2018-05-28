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

cmake_minimum_required(VERSION 3.6)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
    CACHE FILEPATH
    "Toolchain to use for building this package and dependencies")

include(CMakeDependentOption)
option(HUNTER_KEEP_PACKAGE_SOURCES "Keep source code for dependencies" OFF)
cmake_dependent_option(HUNTER_BUILD_SHARED_LIBS
  "Build shared libraries for Hunter packages" OFF
  "NOT BUILD_SHARED_LIBS" ON)

include(HunterGate)
HunterGate(
    URL "https://github.com/ruslo/hunter/archive/v0.20.74.tar.gz"
    SHA1 "88e3c806f0ff640c0e3cbc32b340ae2c370b31f5"
    LOCAL
)

project(jaegertracingc VERSION 0.0.1)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(CheckCCompilerFlag)
include(CheckSymbolExists)
include(CheckStructHasMember)
include(CTest)
include(GenerateExportHeader)
include(TestBigEndian)

include(CheckAtomics)
include(CheckAttributes)
include(CheckBuiltin)
include(Fuzz)
include(GenerateDocumentation)
include(Sanitizers)

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
  append_coverage_compiler_flags(jaegertracingc_flags)
  set(COVERAGE_EXCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/mock_agent.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/mock_agent.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test_driver.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/*_test_driver.h"
    "${CMAKE_CURRENT_BINARY_DIR}/src/jaegertracingc/protoc-gen/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src/*")
endif()

append_sanitizer_flags(sanitizer_flags)
list(APPEND jaegertracingc_flags ${sanitizer_flags})

cmake_dependent_option(JAEGERTRACINGC_VERBOSE_ALLOC "Print all allocations" OFF
                       "BUILD_TESTING;debug_build" OFF)
mark_as_advanced(JAEGERTRACINGC_VERBOSE_ALLOC)

hunter_add_package(opentracing-c)
find_package(opentracing-c CONFIG REQUIRED)
list(APPEND package_deps opentracing-c)

option(HUNTER_ENABLED "Use Hunter build system" ON)

if(HUNTER_ENABLED)
  set(hunter_config "CONFIG")
endif()

hunter_add_package(http-parser)
find_package(http-parser ${hunter_config} REQUIRED)
list(APPEND package_deps http-parser)

hunter_add_package(jansson)
find_package(jansson ${hunter_config} REQUIRED)
list(APPEND package_deps jansson)

if(HUNTER_ENABLED)
  hunter_add_package(Protobuf)
  find_package(Protobuf ${hunter_config} REQUIRED)
  list(APPEND package_deps Protobuf)
  set(protoc_exe protobuf::protoc)
else()
  find_program(protoc_exe protoc)
  if(NOT protoc_exe)
    message(FATAL_ERROR
            "Cannot find protoc executable, please install Protobuf")
  endif()
endif()

hunter_add_package(protobuf-c)
find_package(protobuf-c ${hunter_config} REQUIRED)
list(APPEND package_deps protobuf-c)

hunter_add_package(pcg)
find_package(pcg ${hunter_config} REQUIRED)
list(APPEND package_deps pcg)

option(JAEGERTRACINGC_MT "Enable multithreading support" ON)
if(JAEGERTRACINGC_MT)
  set(CMAKE_THREAD_PREFER_PTHREAD ON)
  find_package(Threads REQUIRED)
  list(APPEND package_deps Threads)
endif()

set(src
  src/jaegertracingc/alloc.c
  src/jaegertracingc/alloc.h
  src/jaegertracingc/baggage.c
  src/jaegertracingc/baggage.h
  src/jaegertracingc/clock.c
  src/jaegertracingc/clock.h
  src/jaegertracingc/common.c
  src/jaegertracingc/common.h
  src/jaegertracingc/constants.c
  ${CMAKE_CURRENT_BINARY_DIR}/src/jaegertracingc/constants.h
  src/jaegertracingc/hashtable.c
  src/jaegertracingc/hashtable.h
  src/jaegertracingc/key_value.c
  src/jaegertracingc/key_value.h
  src/jaegertracingc/list.c
  src/jaegertracingc/list.h
  src/jaegertracingc/log_record.c
  src/jaegertracingc/log_record.h
  src/jaegertracingc/logging.c
  src/jaegertracingc/logging.h
  src/jaegertracingc/metrics.c
  src/jaegertracingc/metrics.h
  src/jaegertracingc/net.c
  src/jaegertracingc/net.h
  src/jaegertracingc/options.c
  src/jaegertracingc/options.h
  src/jaegertracingc/propagation.c
  src/jaegertracingc/propagation.h
  src/jaegertracingc/random.c
  src/jaegertracingc/random.h
  src/jaegertracingc/reporter.c
  src/jaegertracingc/reporter.h
  src/jaegertracingc/sampler.c
  src/jaegertracingc/sampler.h
  src/jaegertracingc/sampling_strategy.c
  src/jaegertracingc/sampling_strategy.h
  src/jaegertracingc/siphash.c
  src/jaegertracingc/siphash.h
  src/jaegertracingc/span.c
  src/jaegertracingc/span.h
  src/jaegertracingc/tag.c
  src/jaegertracingc/tag.h
  src/jaegertracingc/threading.c
  src/jaegertracingc/threading.h
  src/jaegertracingc/token_bucket.c
  src/jaegertracingc/token_bucket.h
  src/jaegertracingc/trace_id.c
  src/jaegertracingc/trace_id.h
  src/jaegertracingc/tracer.c
  src/jaegertracingc/tracer.h
  src/jaegertracingc/vector.c
  src/jaegertracingc/vector.h)

set(generated_src_dir "${CMAKE_CURRENT_BINARY_DIR}/src/jaegertracingc")

file(MAKE_DIRECTORY "${generated_src_dir}/protoc-gen")
file(WRITE "${generated_src_dir}/protoc-gen/.clang-tidy"
     "Checks: 'clang-diagnostic-*,clang-analyzer-*,-clang-diagnostic-unused-command-line-argument'\n")
function(generate_protobuf_c gen_src)
  foreach(proto_src ${ARGN})
    get_filename_component(component ${proto_src} NAME_WE)
    get_filename_component(dir ${proto_src} DIRECTORY)
    set(protoc_out_files
        "${generated_src_dir}/protoc-gen/${component}.pb-c.h"
        "${generated_src_dir}/protoc-gen/${component}.pb-c.c")
    add_custom_command(
      OUTPUT ${protoc_out_files}
      COMMAND ${protoc_exe}
      ARGS --plugin=$<TARGET_FILE:protobuf-c::protoc-gen-c>
           --c_out "${generated_src_dir}/protoc-gen"
           -I. "${component}.proto"
      WORKING_DIRECTORY ${dir}
      MAIN_DEPENDENCY ${proto_src}
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

add_library(jaegertracingc ${src} ${protoc_gen_src})
if(BUILD_SHARED_LIBS AND NOT BUILD_TESTING)
  set_target_properties(jaegertracingc PROPERTIES
    C_VISIBILITY_PRESET hidden
    VISIBILITY_INLINES_HIDDEN ON)
else()
  target_compile_definitions(jaegertracingc PUBLIC JAEGERTRACINGC_STATIC)
endif()
target_compile_options(jaegertracingc PRIVATE ${jaegertracingc_flags})
if(sanitizer_flags)
  target_link_libraries(jaegertracingc PRIVATE ${sanitizer_flags})
endif()

find_program(clang_tidy_path clang-tidy)
if(clang_tidy_path)
  set_target_properties(jaegertracingc PROPERTIES
    C_CLANG_TIDY ${clang_tidy_path})
endif()

generate_export_header(jaegertracingc
  EXPORT_FILE_NAME "${generated_src_dir}/visibility.h")

configure_file(
  src/jaegertracingc/constants.h.in
  "${generated_src_dir}/constants.h"
  @ONLY)
configure_file(
  src/jaegertracingc/config.h.in
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
    "JAEGERTRACINGC_HAVE_ATOMICS")
else()
endif()

check_symbol_exists(
  "SYS_getrandom" "linux/random.h;sys/syscall.h;unistd.h" have_getrandom)
if(have_getrandom)
  target_compile_definitions(jaegertracingc PRIVATE "HAVE_GETRANDOM")
else()
  check_symbol_exists("arc4random_buf" "stdlib.h" have_arc4random_buf)
  if(have_arc4random_buf)
    target_compile_definitions(jaegertracingc PRIVATE "HAVE_ARC4RANDOM_BUF")
  endif()
endif()

test_big_endian(big_endian)
if(NOT big_endian)
  target_compile_definitions(jaegertracingc PRIVATE "LITTLE_ENDIAN")

  if(WIN32)
    set(swap_func "_byteswap_uint64")
    set(swap_func_includes "stdlib.h")
  elseif(APPLE)
    set(swap_func "OSSwapConstInt64")
    set(swap_func_includes "libkern/OSByteOrder.h")
  else()
    set(swap_func "be64toh")
    set(swap_func_includes "endian.h")
  endif()
  check_symbol_exists("${swap_func}" "${swap_func_includes}" have_swap_func)
  if(NOT have_swap_func)
    message(FATAL_ERROR "Cannot find byte order function ${swap_func}")
  endif()
endif()

check_attributes()

check_builtin(have_builtin)
if(have_builtin)
  target_compile_definitions(jaegertracingc PRIVATE HAVE_BUILTIN)
endif()

if(JAEGERTRACINGC_VERBOSE_ALLOC)
  target_compile_definitions(
    jaegertracingc PRIVATE JAEGERTRACINGC_VERBOSE_ALLOC)
endif()

if(BUILD_TESTING)
  add_library(unity "third_party/unity/src/unity.c")
  target_include_directories(unity PUBLIC "third_party/unity/src")

  set(test_src
    src/jaegertracingc/alloc_test.c
    src/jaegertracingc/clock_test.c
    src/jaegertracingc/hashtable_test.c
    src/jaegertracingc/list_test.c
    src/jaegertracingc/logging_test.c
    src/jaegertracingc/log_record_test.c
    src/jaegertracingc/metrics_test.c
    src/jaegertracingc/net_test.c
    src/jaegertracingc/propagation_test.c
    src/jaegertracingc/random_test.c
    src/jaegertracingc/reporter_test.c
    src/jaegertracingc/sampler_test.c
    src/jaegertracingc/siphash_test.c
    src/jaegertracingc/span_test.c
    src/jaegertracingc/tag_test.c
    src/jaegertracingc/threading_test.c
    src/jaegertracingc/trace_id_test.c
    src/jaegertracingc/tracer_test.c
    src/jaegertracingc/token_bucket_test.c
    src/jaegertracingc/vector_test.c)
  foreach(test_file ${test_src})
    get_filename_component(test_component ${test_file} NAME_WE)
    string(REPLACE "_test" "" test_component "${test_component}")
    string(TOUPPER "${test_component}" test_component_upper)
    configure_file(
      src/jaegertracingc/test.h.in
      "${generated_src_dir}/${test_component}_test.h"
      @ONLY)

    set(unit_test_driver_includes
        "${unit_test_driver_includes}\n#include \"${test_component}_test.h\"")
    set(unit_test_driver_impl
        "${unit_test_driver_impl}\n    RUN_TEST(test_${test_component});")
  endforeach()
  configure_file(
    src/jaegertracingc/unit_test_driver.h.in
    "${generated_src_dir}/unit_test_driver.h"
    @ONLY)
  add_library(jaegertracingc_test
    ${test_src}
    src/jaegertracingc/mock_agent.c
    src/jaegertracingc/mock_agent.h)
  target_include_directories(jaegertracingc_test PUBLIC
    "src/jaegertracingc" $<BUILD_INTERFACE:${generated_src_dir}>)
  target_link_libraries(jaegertracingc_test PUBLIC jaegertracingc unity)
  target_compile_definitions(jaegertracingc_test PUBLIC
    UNITY_USE_COMMAND_LINE_ARGS)
  target_compile_options(jaegertracingc_test PUBLIC ${jaegertracingc_flags})
  target_link_libraries(jaegertracingc_test PUBLIC ${sanitizer_flags})

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

check_fuzz_support(have_fuzz_support)
cmake_dependent_option(JAEGERTRACINGC_FUZZ "Enable fuzz testing" OFF
  "BUILD_TESTING;have_fuzz_support" OFF)
if(JAEGERTRACINGC_FUZZ)
  set(fuzz_tests
    src/jaegertracingc/siphash_fuzz_test.c
    src/jaegertracingc/trace_id_fuzz_test.c)
  append_fuzz_flags(fuzz_flags)
  foreach(fuzz_test_src ${fuzz_tests})
    get_filename_component(fuzz_test ${fuzz_test_src} NAME_WE)
    add_executable(${fuzz_test} ${fuzz_test_src})
    target_compile_options(${fuzz_test} PRIVATE ${fuzz_flags})
    target_link_libraries(${fuzz_test} jaegertracingc ${fuzz_flags})
  endforeach()
endif()

option(JAEGERTRACINGC_BUILD_CROSSDOCK "Build crossdock test" OFF)
if(JAEGERTRACINGC_BUILD_CROSSDOCK)
  hunter_add_package(libevhtp)
  find_package(libevhtp CONFIG REQUIRED)
  list(APPEND package_deps libevhtp)

  add_executable(crossdock crossdock/main.c)
  target_link_libraries(crossdock PUBLIC jaegertracingc libevhtp::evhtp)
endif()

generate_documentation()

# Installation (https://github.com/forexample/package-example)

# Introduce variables:
# * CMAKE_INSTALL_LIBDIR
# * CMAKE_INSTALL_BINDIR
# * CMAKE_INSTALL_INCLUDEDIR
include(GNUInstallDirs)

# Layout. This works for all platforms:
#   * <prefix>/lib*/cmake/<PROJECT-NAME>
#   * <prefix>/lib*/
#   * <prefix>/include/
set(config_install_dir "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}")

set(generated_dir "${CMAKE_CURRENT_BINARY_DIR}/generated")

# Configuration
set(version_config "${generated_dir}/${PROJECT_NAME}ConfigVersion.cmake")
set(project_config "${generated_dir}/${PROJECT_NAME}Config.cmake")
set(TARGETS_EXPORT_NAME "${PROJECT_NAME}Targets")
set(namespace "${PROJECT_NAME}::")

# Include module with fuction 'write_basic_package_version_file'
include(CMakePackageConfigHelpers)

# Configure '<PROJECT-NAME>ConfigVersion.cmake'
# Use:
#   * PROJECT_VERSION
write_basic_package_version_file(
    "${version_config}" COMPATIBILITY SameMajorVersion
)

# Configure '<PROJECT-NAME>Config.cmake'
# Use variables:
#   * TARGETS_EXPORT_NAME
#   * PROJECT_NAME
configure_package_config_file(
    "cmake/Config.cmake.in"
    "${project_config}"
    INSTALL_DESTINATION "${config_install_dir}"
)

# Targets:
#   * <prefix>/lib/libjaegertracingc.a
#   * header location after install: <prefix>/include/jaegertracingc/tracer.h
#   * headers can be included by C++ code `#include <jaegertracingc/tracer.h>`
install(
    TARGETS jaegertracingc
    EXPORT "${TARGETS_EXPORT_NAME}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
)

# Headers:
#   * src/jaegertracingc/tracer.h -> <prefix>/include/jaegertracingc/tracer.h
#   * src/jaegertracingc/span.h -> <prefix>/include/jaegertracingc/span.h
install(
    DIRECTORY "src/jaegertracingc"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h"
)

# Export headers:
#   * ${CMAKE_CURRENT_BINARY_DIR}/.../visibility.h -> <prefix>/include/jaegertracingc/visibility.h
install(
    FILES "${generated_src_dir}/visibility.h"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/jaegertracingc"
)

# Config
#   * <prefix>/lib/cmake/jaegertracingc/jaegertracingcConfig.cmake
#   * <prefix>/lib/cmake/jaegertracingc/jaegertracingcConfigVersion.cmake
install(
    FILES "${project_config}" "${version_config}"
    DESTINATION "${config_install_dir}"
)

# Config
#   * <prefix>/lib/cmake/jaegertracingc/jaegertracingcTargets.cmake
install(
    EXPORT "${TARGETS_EXPORT_NAME}"
    NAMESPACE "${namespace}"
    DESTINATION "${config_install_dir}"
)
