if(__CHECK_ATOMICS)
  return()
endif()
set(__CHECK_ATOMICS 1)

function(check_atomics var)
  try_compile(have_atomics
    "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp/atomics_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/atomics_test.c")
  if(have_atomics)
    message(STATUS "Checking for atomics - Success")
    set(${var} ON PARENT_SCOPE)
  else()
    message(STATUS "Checking for atomics - Failure")
  endif()
endfunction()
