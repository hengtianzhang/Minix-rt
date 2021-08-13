# SPDX-License-Identifier: Apache-2.0

set_ifndef(CC gcc)
set_ifndef(C++ g++)

find_program(CMAKE_C_COMPILER ${CC})
if (NOT CMAKE_C_COMPILER)
	message(FATAL_ERROR "C compiler ${CC} not found - Please check your toolchain installation")
endif()

find_program(CMAKE_CXX_COMPILER ${C++})
if (NOT CMAKE_CXX_COMPILER)
	message(FATAL_ERROR "C compiler ${C++} not found - Please check your toolchain installation")
endif()
