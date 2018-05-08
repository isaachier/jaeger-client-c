if(__CHECK_ATTRIBUTES)
  return()
endif()
set(__CHECK_ATTRIBUTES 1)

function(__check_specific_attribute specific_attribute var)
  set(tmp_dir "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp")
  try_compile(have_${specific_attribute}_attribute
    "${tmp_dir}/${specific_attribute}_attribute_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/specific_attribute_test.c"
    COMPILE_DEFINITIONS "-DATTRIBUTE=${JAEGERTRACINGC_ATTRIBUTE}"
                        "-DX=${specific_attribute}"
                         "-Wall" "-Werror")

  if(have_${specific_attribute}_attribute)
    set(${var} ON PARENT_SCOPE)
    message(STATUS "Checking for ${JAEGERTRACINGC_ATTRIBUTE}((${specific_attribute})) - Success")
  else()
    message(STATUS "Checking for ${JAEGERTRACINGC_ATTRIBUTE}((${specific_attribute})) - Failed")
  endif()
endfunction()

function(__check_have_weak_symbols var)
  __check_specific_attribute("weak" have_weak_symbols)

  if(have_weak_symbols)
    set(${var} ON PARENT_SCOPE)
  endif()
endfunction()

function(__check_have_nonnull_attribute var)
  __check_specific_attribute("nonnull" have_nonnull_attribute)
  if(have_nonnull_attribute)
    set(${var} ON PARENT_SCOPE)
  endif()
endfunction()

function(__check_have_format_attribute var)
  set(tmp_dir "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp")
  try_compile(have_format_attribute
    "${tmp_dir}/cmake/format_attribute_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/format_attribute_test.c"
    COMPILE_DEFINITIONS "-DATTRIBUTE=${JAEGERTRACINGC_ATTRIBUTE}"
                        "-Wall" "-Werror")
  if(have_format_attribute)
    set(${var} ON PARENT_SCOPE)
    message(STATUS "Checking for ${JAEGERTRACINGC_ATTRIBUTE}((format())) - Success")
  else()
    message(STATUS "Checking for ${JAEGERTRACINGC_ATTRIBUTE}((format())) - Failed")
  endif()
endfunction()

function(__check_have_guarded_by_attribute var)
  find_package(Threads REQUIRED)
  set(tmp_dir "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp")
  try_compile(have_guarded_by_attribute
    "${tmp_dir}/cmake/guarded_by_attribute_test"
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/guarded_by_attribute_test.c"
    COMPILE_DEFINITIONS "-DATTRIBUTE=${JAEGERTRACINGC_ATTRIBUTE}"
                        "-Wall" "-Werror"
    LINK_LIBRARIES Threads::Threads)
  if(have_guarded_by_attribute)
    set(${var} ON PARENT_SCOPE)
    message(STATUS "Checking for ${JAEGERTRACINGC_ATTRIBUTE}((guarded_by())) - Success")
  else()
    message(STATUS "Checking for ${JAEGERTRACINGC_ATTRIBUTE}((guarded_by())) - Failed")
  endif()
endfunction()

function(check_attributes)
  set(tmp_dir "${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY}/CMakeTmp")
  if(NOT DEFINED JAEGERTRACINGC_ATTRIBUTE)
    foreach(keyword "__attribute__" "__attribute")
      try_compile(have_${keyword}
        "${tmp_dir}/attribute_keyword_test"
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/attribute_keyword_test.c"
        COMPILE_DEFINITIONS "-DATTRIBUTE=${keyword}" "-Wall" "-Werror")
      if(have_${keyword})
        set(JAEGERTRACINGC_ATTRIBUTE "${keyword}" CACHE INTERNAL
            "Attribute keyword")
        break()
      endif()
    endforeach()
  endif()

  if(NOT JAEGERTRACINGC_ATTRIBUTE)
    return()
  endif()

  __check_have_weak_symbols(have_weak_symbols)
  if(have_weak_symbols)
    set(JAEGERTRACINGC_HAVE_WEAK_SYMBOLS ON PARENT_SCOPE)
  endif()

  __check_have_nonnull_attribute(have_nonnull_attribute)
  if(have_nonnull_attribute)
    set(JAEGERTRACINGC_HAVE_NONNULL_ATTRIBUTE ON PARENT_SCOPE)
  endif()

  __check_have_format_attribute(have_format_attribute)
  if(have_format_attribute)
    set(JAEGERTRACINGC_HAVE_FORMAT_ATTRIBUTE ON PARENT_SCOPE)
  endif()

  __check_have_guarded_by_attribute(have_guarded_by_attribute)
  if(have_guarded_by_attribute)
    set(JAEGERTRACINGC_HAVE_GUARDED_BY_ATTRIBUTE ON PARENT_SCOPE)
  endif()
endfunction()
