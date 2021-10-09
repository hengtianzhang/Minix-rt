# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC gcc)
set_ifndef(C++ g++)

# Configures CMake for using GCC, this script is re-used by several
# GCC-based toolchains
if(DEFINED TOOLCHAIN_HOME)
  set(find_program_gcc_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}${CC} ${find_program_gcc_args})
if(${CMAKE_C_COMPILER} STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "C compiler ${CROSS_COMPILE}${CC} not found - Please check your toolchain installation")
endif()

find_program(CMAKE_CXX_COMPILER ${CROSS_COMPILE}${C++} ${find_program_gcc_args})
if(${CMAKE_CXX_COMPILER} STREQUAL CMAKE_CXX_COMPILER-NOTFOUND)
  message(FATAL_ERROR "C compiler ${CROSS_COMPILE}${C++} not found - Please check your toolchain installation")
endif()

set(NOSTDINC "")

execute_process(
	COMMAND ${CMAKE_C_COMPILER} --print-file-name=include
	OUTPUT_VARIABLE _OUTPUT
)

string(REGEX REPLACE "\n" "" _OUTPUT "${_OUTPUT}")

list(APPEND NOSTDINC ${_OUTPUT})

# For CMake to be able to test if a compiler flag is supported by the
# toolchain we need to give CMake the necessary flags to compile and
# link a dummy C file.
#
# CMake checks compiler flags with check_c_compiler_flag() (Which we
# wrap with target_cc_option() in extentions.cmake)
foreach(isystem_include_dir ${NOSTDINC})
  list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()

# The CMAKE_REQUIRED_FLAGS variable is used by check_c_compiler_flag()
# (and other commands which end up calling check_c_source_compiles())
# to add additional compiler flags used during checking. These flags
# are unused during "real" builds of minix_rt source files linked into
# the final executable.
#
# Appending onto any existing values lets users specify
# toolchain-specific flags at generation time.
list(APPEND CMAKE_REQUIRED_FLAGS
  -nostartfiles
  -nostdlib
  ${isystem_include_flags}
  -Wl,--unresolved-symbols=ignore-in-object-files
  -Wl,--entry=0 # Set an entry point to avoid a warning
  )
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")
