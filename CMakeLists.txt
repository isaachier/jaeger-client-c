cmake_minimum_required(VERSION 3.1)

project(jaegertracingc VERSION 0.0.1 LANGUAGES C)

find_package(Threads REQUIRED)

add_library(sds "thirdparty/sds/sds.c")
target_include_directories(sds PUBLIC "thirdparty/sds")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(jaegertracingc
    "src/jaegertracingc/alloc.c"
    "src/jaegertracingc/common.c"
    "src/jaegertracingc/sampler.c"
    "src/jaegertracingc/span.c"
    "src/jaegertracingc/time.c"
    "src/jaegertracingc/token_bucket.c")
target_include_directories(jaegertracingc PUBLIC "src")
target_link_libraries(jaegertracingc PUBLIC sds)
