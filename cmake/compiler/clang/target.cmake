# SPDX-License-Identifier: Apache-2.0

if(DEFINED TOOLCHAIN_HOME)
	set(find_program_clang_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER   clang   ${find_program_clang_args})
find_program(CMAKE_CXX_COMPILER clang++ ${find_program_clang_args})

# Configuration for host installed clang
#
set(NOSTDINC "")

execute_process(
	COMMAND ${CMAKE_C_COMPILER} --print-file-name=include
	OUTPUT_VARIABLE _OUTPUT
)

string(REGEX REPLACE "\n" "" _OUTPUT "${_OUTPUT}")

list(APPEND NOSTDINC ${_OUTPUT})

foreach(isystem_include_dir ${NOSTDINC})
	list(APPEND isystem_include_flags -isystem "\"${isystem_include_dir}\"")
endforeach()

set(CMAKE_REQUIRED_FLAGS -nostartfiles -nostdlib ${isystem_include_flags})
string(REPLACE ";" " " CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS}")

if((NOT (DEFINED CROSS_COMPILE)) AND (DEFINED ENV{CROSS_COMPILE}))
	set(CROSS_COMPILE $ENV{CROSS_COMPILE})
endif()

set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "")
assert(CROSS_COMPILE "CROSS_COMPILE is not set")

string(REGEX REPLACE "-$" "" triple ${CROSS_COMPILE})

set(CMAKE_C_COMPILER_TARGET   ${triple})
set(CMAKE_ASM_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER_TARGET ${triple})
