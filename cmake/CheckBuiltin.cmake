if(__CHECK_BUILTIN)
  return()
endif()
set(__CHECK_BUILTIN 1)

function(check_builtin var)
  try_compile(have_builtin
    "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/builtin_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/builtin_test.c")
  if(have_builtin)
    message(STATUS "Checking for builtin functions - Success")
    set(${var} ON PARENT_SCOPE)
  else()
    message(STATUS "Checking for builtin functions - Failure")
  endif()
endfunction()
