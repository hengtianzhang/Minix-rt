# SPDX-License-Identifier: Apache-2.0

find_program(CMAKE_C_COMPILER clang)
if (NOT CMAKE_C_COMPILER)
	message(FATAL_ERROR "C compiler clang not found - Please check your toolchain installation")
endif()

find_program(CMAKE_CXX_COMPILER clang++)
if (NOT CMAKE_CXX_COMPILER)
	message(FATAL_ERROR "C compiler clang++ not found - Please check your toolchain installation")
endif()
