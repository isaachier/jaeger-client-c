cmake_minimum_required(VERSION 3.1)

project(jaegertracingc VERSION 0.0.1 LANGUAGES C)

find_package(Threads REQUIRED)

add_library(sds "thirdparty/sds/sds.c")
target_include_directories(sds PUBLIC "thirdparty/sds")

set(CMAKE_INCLUDE_CURRENT_DIR ON)

add_library(jaegertracingc "span.c")
target_link_libraries(jaegertracingc PUBLIC sds)
