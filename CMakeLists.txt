cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
    CACHE FILEPATH
    "Toolchain to use for building this package and dependencies")

project(jaegertracingc VERSION 0.0.1 LANGUAGES C)

include(CheckCCompilerFlag)
include(CheckFunctionExists)
include(CMakeDependentOption)
include(CTest)

foreach(flag "-Wall" "-Werror")
  string(MAKE_C_IDENTIFIER "HAVE${flag}" flag_var)
  check_c_compiler_flag(${flag} ${flag_var})
  if(${flag_var})
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${flag}")
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
    "${CMAKE_CURRENT_SOURCE_DIR}/src/jaegertracingc/protoc-gen/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/third_party/unity/src/*")
endif()

cmake_dependent_option(JAEGERTRACINGC_VERBOSE_ALLOC "Print all allocations" OFF
                       "BUILD_TESTING;debug_build" OFF)
mark_as_advanced(JAEGERTRACINGC_VERBOSE_ALLOC)

find_package(http-parser REQUIRED)
find_package(Jansson REQUIRED)
find_package(ProtobufC REQUIRED)

option(JAEGERTRACINGC_MT "Enable multithreading support" ON)
if(JAEGERTRACINGC_MT)
  set(CMAKE_THREAD_PREFER_PTHREAD ON)
  find_package(Threads REQUIRED)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(core_src
  "src/jaegertracingc/alloc.c"
  "src/jaegertracingc/alloc.h"
  "src/jaegertracingc/common.c"
  "src/jaegertracingc/common.h"
  "src/jaegertracingc/constants.c"
  "src/jaegertracingc/duration.c"
  "src/jaegertracingc/duration.h"
  "src/jaegertracingc/init.c"
  "src/jaegertracingc/init.h"
  "src/jaegertracingc/logging.c"
  "src/jaegertracingc/logging.h"
  "src/jaegertracingc/metrics.c"
  "src/jaegertracingc/metrics.h"
  "src/jaegertracingc/net.c"
  "src/jaegertracingc/net.h"
  "src/jaegertracingc/sampler.c"
  "src/jaegertracingc/sampler.h"
  "src/jaegertracingc/sampling_strategy.c"
  "src/jaegertracingc/sampling_strategy.h"
  "src/jaegertracingc/tag.c"
  "src/jaegertracingc/tag.h"
  "src/jaegertracingc/threading.c"
  "src/jaegertracingc/threading.h"
  "src/jaegertracingc/token_bucket.c"
  "src/jaegertracingc/token_bucket.h"
  "src/jaegertracingc/types.c"
  "src/jaegertracingc/types.h")

set(protoc_gen_src
  "src/jaegertracingc/protoc-gen/agent.pb-c.c"
  "src/jaegertracingc/protoc-gen/agent.pb-c.h"
  "src/jaegertracingc/protoc-gen/baggage.pb-c.c"
  "src/jaegertracingc/protoc-gen/baggage.pb-c.h"
  "src/jaegertracingc/protoc-gen/jaeger.pb-c.c"
  "src/jaegertracingc/protoc-gen/jaeger.pb-c.h"
  "src/jaegertracingc/protoc-gen/sampling.pb-c.c"
  "src/jaegertracingc/protoc-gen/sampling.pb-c.h")

add_library(jaegertracingc ${core_src} ${protoc_gen_src})

configure_file(
  "src/jaegertracingc/constants.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/include/jaegertracingc/constants.h"
  @ONLY)
target_include_directories(jaegertracingc PUBLIC
  "src" "${CMAKE_CURRENT_BINARY_DIR}/include")

target_link_libraries(jaegertracingc PUBLIC
  http-parser::http-parser
  Jansson::Jansson
  ProtobufC::ProtobufC)
if(JAEGERTRACINGC_MT)
  target_link_libraries(jaegertracingc PUBLIC Threads::Threads)
  target_compile_definitions(jaegertracingc PUBLIC JAEGERTRACINGC_MT)
endif()

check_function_exists("rand_r" have_rand_r)
if(have_rand_r)
  target_compile_definitions(jaegertracingc PUBLIC "-DHAVE_RAND_R")
else()
  message(WARN
    "Cannot find rand_r, so using rand, which may negatively affect "
    "probabilistic sampling")
endif()

if(JAEGERTRACINGC_VERBOSE_ALLOC)
  target_compile_definitions(
    jaegertracingc PUBLIC "-DJAEGERTRACINGC_VERBOSE_ALLOC")
endif()

if(BUILD_TESTING)
  add_library(unity "third_party/unity/src/unity.c")
  target_include_directories(unity PUBLIC "third_party/unity/src")

  set(test_src
    "src/jaegertracingc/alloc_test.c"
    "src/jaegertracingc/duration_test.c"
    "src/jaegertracingc/metrics_test.c"
    "src/jaegertracingc/net_test.c"
    "src/jaegertracingc/sampler_test.c"
    "src/jaegertracingc/tag_test.c"
    "src/jaegertracingc/token_bucket_test.c")
  add_library(jaegertracingc_test ${test_src})
  target_link_libraries(jaegertracingc_test PUBLIC jaegertracingc unity)

  add_executable(default_test "src/jaegertracingc/default_test_driver.c")
  target_link_libraries(default_test PUBLIC jaegertracingc_test)
  add_test(default_test default_test)

  add_executable(oom_test "src/jaegertracingc/oom_test_driver.c")
  target_link_libraries(oom_test PUBLIC jaegertracingc_test)
  add_test(oom_test oom_test)

  if(JAEGERTRACINGC_COVERAGE)
    setup_target_for_coverage(
      NAME unit_test_coverage
      EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/default_test &&
								 ${CMAKE_CURRENT_BINARY_DIR}/oom_test
      DEPENDENCIES default_test oom_test)
  endif()
endif()

find_program(doxygen "doxygen")
if(doxygen)
  message(STATUS "Found doxygen: ${doxygen}")
else()
  message(STATUS "Will not build documentation because doxygen was not found")
endif()

find_program(sphinx_build "sphinx-build")
if(sphinx_build)
  message(STATUS "Found sphinx-build: ${sphinx_build}")
else()
  message(STATUS
    "Will not build documentation because sphinx-build was not found")
endif()
cmake_dependent_option(JAEGERTRACINGC_BUILD_DOC "Build documentation" ON
                       "doxygen;sphinx_build" OFF)
if(JAEGERTRACINGC_BUILD_DOC)
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

	configure_file("${sphinx_src_path}/conf.py.in"
                 "${sphinx_out_path}/conf.py" @ONLY)
	configure_file("${sphinx_src_path}/index.rst.in"
                 "${sphinx_out_path}/index.rst" @ONLY)
  add_custom_target(doc
    COMMAND ${sphinx_build} -M html ${sphinx_out_path} ${sphinx_out_path}
		VERBATIM
		USES_TERMINAL)
endif()
