if(__CHECK_FORMAT_ATTRIBUTE)
  return()
endif()
set(__CHECK_FORMAT_ATTRIBUTE 1)

function(check_format_attribute var)
  try_compile(have_format_attribute
    "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/format_attribute_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/format_attribute_test.c")
  if(have_format_attribute)
    message(STATUS "Checking for __attribute__(format) - Success")
    set(${var} ON PARENT_SCOPE)
  else()
    message(STATUS "Checking for __attribute__(format) - Failed")
  endif()
endfunction()
