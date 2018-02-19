cmake_minimum_required(VERSION 3.1)

project(protobuf_test_project C)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/..")

find_package(ProtobufC REQUIRED)

function(generate_protobuf_c gen_src)
  foreach(proto_src ${ARGN})
    get_filename_component(component ${proto_src} NAME_WE)
    set(protoc_out_files "${CMAKE_CURRENT_BINARY_DIR}/${component}.pb-c.h"
                         "${CMAKE_CURRENT_BINARY_DIR}/${component}.pb-c.c")
    add_custom_command(
      OUTPUT ${protoc_out_files}
      COMMAND ${PROTOC_C_EXECUTABLE} --c_out "${CMAKE_CURRENT_BINARY_DIR}"
              -I. ${proto_src}
      MAIN_DEPENDENCY ${proto_src}
      DEPENDENCIES ${PROTOC_C_EXECUTABLE}
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      VERBATIM
      USES_TERMINAL)
    list(APPEND out_files ${protoc_out_files})
  endforeach()
  set(${gen_src} ${out_files} PARENT_SCOPE)
endfunction()

generate_protobuf_c(proto_gen_src optional_test.proto)
message(STATUS "proto_gen_src: ${proto_gen_src}")
add_executable(optional_test "optional_test.c" ${proto_gen_src})
target_link_libraries(optional_test PUBLIC ProtobufC::ProtobufC)
