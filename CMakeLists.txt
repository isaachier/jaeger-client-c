cmake_minimum_required(VERSION 3.1)

set(CMAKE_TOOLCHAIN_FILE "${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain.cmake"
    CACHE FILEPATH
    "Toolchain to use for building this package and dependencies")

project(jaegertracingc VERSION 0.0.1 LANGUAGES C)

include(CTest)

find_package(Threads REQUIRED)

add_library(sds "thirdparty/sds/sds.c")
target_include_directories(sds PUBLIC "thirdparty/sds")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(jaegertracingc
    "src/jaegertracingc/alloc.c"
    "src/jaegertracingc/common.c"
    "src/jaegertracingc/duration.c"
    "src/jaegertracingc/sampler.c"
    "src/jaegertracingc/span.c"
    "src/jaegertracingc/token_bucket.c")
target_include_directories(jaegertracingc PUBLIC "src")
target_link_libraries(jaegertracingc PUBLIC sds)

if(BUILD_TESTING)
  add_library(unity "thirdparty/unity/unity.c")
  target_include_directories(unity PUBLIC "thirdparty/unity")

  set(test_src
      "src/jaegertracingc/token_bucket_test.c")
  foreach(src ${test_src})
    get_filename_component(name ${src} NAME_WE)
    add_executable(${name} ${src})
    target_link_libraries(${name} PUBLIC jaegertracingc unity)
  endforeach()
endif()
