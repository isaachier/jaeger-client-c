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
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/unity/src/*")
endif()

cmake_dependent_option(JAEGERTRACINGC_VERBOSE_ALLOC "Print all allocations" OFF
                       "BUILD_TESTING;debug_build" OFF)
mark_as_advanced(JAEGERTRACINGC_VERBOSE_ALLOC)

find_package(http-parser REQUIRED)
find_package(Jansson REQUIRED)
find_package(ProtobufC REQUIRED)

set(CMAKE_THREAD_PREFER_PTHREAD ON)
find_package(Threads REQUIRED)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(jaegertracingc
  "src/jaegertracingc/alloc.c"
  "src/jaegertracingc/common.c"
  "src/jaegertracingc/constants.c"
  "src/jaegertracingc/duration.c"
  "src/jaegertracingc/init.c"
  "src/jaegertracingc/metrics.c"
  "src/jaegertracingc/sampler.c"
  "src/jaegertracingc/sampling_strategy.c"
  "src/jaegertracingc/tag.c"
  "src/jaegertracingc/ticker.c"
  "src/jaegertracingc/token_bucket.c"
  "src/jaegertracingc/types.c"
  "src/jaegertracingc/protoc-gen/agent.pb-c.c"
  "src/jaegertracingc/protoc-gen/baggage.pb-c.c"
  "src/jaegertracingc/protoc-gen/jaeger.pb-c.c"
  "src/jaegertracingc/protoc-gen/sampling.pb-c.c")

configure_file(
  "src/jaegertracingc/constants.h.in"
  "${CMAKE_CURRENT_BINARY_DIR}/generated/include/jaegertracingc/constants.h"
  @ONLY)
target_include_directories(jaegertracingc PUBLIC
  "src" "${CMAKE_CURRENT_BINARY_DIR}/generated/include")

if(CMAKE_USE_PTHREADS_INIT)
  target_link_libraries(jaegertracingc PUBLIC
    http-parser::http-parser
    Jansson::Jansson
    ProtobufC::ProtobufC
    Threads::Threads)
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
  add_library(unity "thirdparty/unity/src/unity.c")
  target_include_directories(unity PUBLIC "thirdparty/unity/src")

  set(TEST_SRC
    "src/jaegertracingc/alloc_test.c"
    "src/jaegertracingc/duration_test.c"
    "src/jaegertracingc/metrics_test.c"
    "src/jaegertracingc/sampler_test.c"
    "src/jaegertracingc/tag_test.c"
    "src/jaegertracingc/ticker_test.c"
    "src/jaegertracingc/token_bucket_test.c")
  add_library(jaegertracingc_test ${TEST_SRC})
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
      EXECUTABLE ${CMAKE_CURRENT_BINARY_DIR}/default_test && ${CMAKE_CURRENT_BINARY_DIR}/oom_test
      DEPENDENCIES default_test oom_test)
  endif()
endif()
