#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "jaegertracingc::jaegertracingc" for configuration "Debug"
set_property(TARGET jaegertracingc::jaegertracingc APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(jaegertracingc::jaegertracingc PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "C"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/libjaegertracingcd.a"
  )

list(APPEND _IMPORT_CHECK_TARGETS jaegertracingc::jaegertracingc )
list(APPEND _IMPORT_CHECK_FILES_FOR_jaegertracingc::jaegertracingc "${_IMPORT_PREFIX}/lib/libjaegertracingcd.a" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
