if(__toolchain)
  return()
endif()
set(__toolchain ON)

include(CheckCCompilerFlag)

set(CMAKE_C_STANDARD_REQUIRED true)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_EXTENSIONS ON)
