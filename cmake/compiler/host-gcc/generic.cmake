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
