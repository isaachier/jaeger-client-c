if(__FORMAT_ATTRIBUTE)
  return()
endif()
set(__FORMAT_ATTRIBUTE 1)

function(format_attribute var)
  try_compile(has_format_attribute
    "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/format_attribute_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/format_attribute_test.c")
  if(has_format_attribute)
    set(${var} ON PARENT_SCOPE)
  endif()
endfunction()
