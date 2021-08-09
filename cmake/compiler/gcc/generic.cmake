# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC gcc)

if(DEFINED TOOLCHAIN_HOME)
  set(find_program_gcc_args PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_C_COMPILER ${CROSS_COMPILE}${CC}   ${find_program_gcc_args})

if(CMAKE_C_COMPILER STREQUAL CMAKE_C_COMPILER-NOTFOUND)
  message(FATAL_ERROR "sel4m was unable to find the toolchain. Is the environment misconfigured?
User-configuration:
SEL4M_TOOLCHAIN: ${SEL4M_TOOLCHAIN}
Internal variables:
CROSS_COMPILE: ${CROSS_COMPILE}
TOOLCHAIN_HOME: ${TOOLCHAIN_HOME}
")
endif()

execute_process(
  COMMAND ${CMAKE_C_COMPILER} --version
  RESULT_VARIABLE ret
  OUTPUT_QUIET
  ERROR_QUIET
  )
if(ret)
  message(FATAL_ERROR "Executing the below command failed. Are permissions set correctly?
'${CMAKE_C_COMPILER} --version'
"
    )
endif()
