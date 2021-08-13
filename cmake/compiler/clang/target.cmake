# SPDX-License-Identifier: Apache-2.0

# Configuration for host installed clang
#

set(NOSTDINC "")

# Note that NOSYSDEF_CFLAG may be an empty string, and
# set_ifndef() does not work with empty string.
if(NOT DEFINED NOSYSDEF_CFLAG)
  set(NOSYSDEF_CFLAG -undef)
endif()

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER   clang   ${find_program_clang_args})
find_program(CMAKE_CXX_COMPILER clang++ ${find_program_clang_args})

if(NOT "${ARCH}" STREQUAL "posix")
  include(${SEL4M_BASE}/cmake/gcc-m-cpu.cmake)

  if("${ARCH}" STREQUAL "arm")
    list(APPEND TOOLCHAIN_C_FLAGS
      -fshort-enums
      )
    list(APPEND TOOLCHAIN_LD_FLAGS
      -fshort-enums
      )

    include(${SEL4M_BASE}/cmake/compiler/gcc/target_arm.cmake)
  endif()

  foreach(file_name include/stddef.h)
    execute_process(
      COMMAND ${CMAKE_C_COMPILER} --print-file-name=${file_name}
      OUTPUT_VARIABLE _OUTPUT
      )
    get_filename_component(_OUTPUT "${_OUTPUT}" DIRECTORY)
    string(REGEX REPLACE "\n" "" _OUTPUT ${_OUTPUT})

    list(APPEND NOSTDINC ${_OUTPUT})
  endforeach()

  foreach(isystem_include_dir ${NOSTDINC})
    list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
  endforeach()

  if(CONFIG_X86)
    if(CONFIG_64BIT)
      string(APPEND TOOLCHAIN_C_FLAGS "-m64")
    else()
      string(APPEND TOOLCHAIN_C_FLAGS "-m32")
    endif()
  endif()

  # This libgcc code is partially duplicated in compiler/*/target.cmake
  execute_process(
    COMMAND ${CMAKE_C_COMPILER} ${TOOLCHAIN_C_FLAGS} --print-libgcc-file-name
    OUTPUT_VARIABLE LIBGCC_FILE_NAME
    OUTPUT_STRIP_TRAILING_WHITESPACE
    )

  get_filename_component(LIBGCC_DIR ${LIBGCC_FILE_NAME} DIRECTORY)

  list(APPEND LIB_INCLUDE_DIR "-L\"${LIBGCC_DIR}\"")
  if(LIBGCC_DIR)
    list(APPEND TOOLCHAIN_LIBS gcc)
  endif()

  set(CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags})
  string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

endif()

# Load toolchain_cc-family macros

macro(toolchain_cc_nostdinc location)
  if(NOT "${ARCH}" STREQUAL "posix")
    sel4m_compile_options( -nostdinc ${location})
  endif()
endmacro()

if((NOT (DEFINED CROSS_COMPILE)) AND (DEFINED ENV{CROSS_COMPILE}))
  set(CROSS_COMPILE $ENV{CROSS_COMPILE})
endif()

set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "")
assert(CROSS_COMPILE "CROSS_COMPILE is not set")

string(REGEX REPLACE "-$" "" triple ${CROSS_COMPILE})

set(CMAKE_C_COMPILER_TARGET   ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})
