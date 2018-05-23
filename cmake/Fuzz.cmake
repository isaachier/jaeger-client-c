if(__FUZZ)
  return()
endif()
set(__FUZZ ON)

set(_fuzz_flags "-fsanitize=fuzzer,address,undefined")

function(append_fuzz_flags var)
  list(APPEND ${var} ${_fuzz_flags})
  set(${var} "${${var}}" PARENT_SCOPE)
endfunction()

function(check_fuzz_support var)
  set(tmp_dir "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp")
  try_compile(have_fuzz_support
    "${tmp_dir}/fuzz_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/fuzz_test.c"
    LINK_LIBRARIES ${_fuzz_flags})
  if(have_fuzz_support)
    set(${var} ON PARENT_SCOPE)
    message(STATUS "Checking for fuzz support - Success")
  else()
    message(STATUS "Checking for fuzz support - Failed")
  endif()
endfunction()
