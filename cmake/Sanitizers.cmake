if(__SANITIZERS)
  return()
endif()
set(__SANITIZERS ON)

function(append_sanitizer_flags flags_var)
  option(ADDRESS_SANITIZER "Enable address sanitizer" OFF)
  option(LEAK_SANITIZER "Enable leak sanitizer" OFF)
  option(THREAD_SANITIZER "Enable thread sanitizer" OFF)
  option(UNDEFINED_SANITIZER "Enable undefined behavior sanitizer" OFF)

  if(ADDRESS_SANITIZER)
    list(APPEND sanitizers address)
  endif()

  if(LEAK_SANITIZER)
    list(APPEND sanitizers leak)
  endif()

  if(THREAD_SANITIZER)
    list(APPEND sanitizers thread)
  endif()

  if(UNDEFINED_SANITIZER)
    list(APPEND sanitizers undefined)
  endif()

  if(sanitizers)
    string(REPLACE ";" "," flag "${sanitizers}")
    set(flag -fsanitize=${flag})
    list(APPEND "${flags_var}" ${flag})
    set("${flags_var}" "${${flags_var}}" PARENT_SCOPE)
  endif()
endfunction()
