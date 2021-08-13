# SPDX-License-Identifier: Apache-2.0

# Configures CMake for using GCC

find_program(CMAKE_C_COMPILER gcc)
if (NOT CMAKE_C_COMPILER)
	message(FATAL_ERROR "C compiler gcc not found - Please check your toolchain installation")
endif()

find_program(CMAKE_CXX_COMPILER g++)
if (NOT CMAKE_CXX_COMPILER)
	message(FATAL_ERROR "C compiler g++ not found - Please check your toolchain installation")
endif()

# The x32 version of libgcc is usually not available (can't trust gcc
# -mx32 --print-libgcc-file-name) so don't fail to build for something
# that is currently not needed. See comments in compiler/gcc/target.cmake
if (CONFIG_X86)
  # Convert to list as cmake Modules/*.cmake do it
  STRING(REGEX REPLACE " +" ";" PRINT_LIBGCC_ARGS "${CMAKE_C_FLAGS}")
  # This libgcc code is partially duplicated in compiler/*/target.cmake
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} "${PRINT_LIBGCC_ARGS}" --print-libgcc-file-name
    OUTPUT_VARIABLE LIBGCC_FILE_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )
  assert_exists(LIBGCC_FILE_NAME)
endif()

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

foreach(file_name include/stddef.h)
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
    OUTPUT_VARIABLE _OUTPUT
    )
  get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
  string(REGEX REPLACE "\n" "" _OUTPUT "${_OUTPUT}")

  list(APPEND NOSTDINC ${_OUTPUT})
endforeach()
