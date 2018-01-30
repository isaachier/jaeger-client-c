cmake_minimum_required(VERSION 3.1)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
    CACHE FILEPATH
    "Toolchain to use for building this package and dependencies")

project(jaegertracingc VERSION 0.0.1 LANGUAGES C)

include(CTest)

option(JAEGERTRACINGC_COVERAGE "Enable code coverage" OFF)

if(BUILD_TESTING AND CMAKE_BUILD_TYPE STREQUAL "Debug" AND
   JAEGERTRACINGC_COVERAGE)
  include(CodeCoverage)
  append_coverage_compiler_flags()
  set(COVERAGE_EXCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/sds/*"
    "${CMAKE_CURRENT_SOURCE_DIR}/thirdparty/unity/*")
endif()

find_package(Threads REQUIRED)

add_library(sds "thirdparty/sds/sds.c")
target_include_directories(sds PUBLIC "thirdparty/sds")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(jaegertracingc
    "src/jaegertracingc/alloc.c"
    "src/jaegertracingc/common.c"
    "src/jaegertracingc/duration.c"
    "src/jaegertracingc/logger.c"
    "src/jaegertracingc/sampler.c"
    "src/jaegertracingc/span.c"
    "src/jaegertracingc/trace_id.c"
    "src/jaegertracingc/token_bucket.c")
target_include_directories(jaegertracingc PUBLIC "src")
target_link_libraries(jaegertracingc PUBLIC sds)

if(BUILD_TESTING)
  add_library(unity "thirdparty/unity/unity.c")
  target_include_directories(unity PUBLIC "thirdparty/unity")

  add_executable(unit_test
    "src/jaegertracingc/unit_test.c"
    "src/jaegertracingc/token_bucket_test.c")
  target_link_libraries(unit_test PUBLIC jaegertracingc unity)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND JAEGERTRACINGC_COVERAGE)
    setup_target_for_coverage(
      NAME unit_test_coverage
      EXECUTABLE unit_test
      DEPENDENCIES unit_test)
  endif()

  add_test(unit_test unit_test)
endif()
