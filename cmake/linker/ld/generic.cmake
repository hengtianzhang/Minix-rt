# SPDX-License-Identifier: Apache-2.0

find_program(CMAKE_LINKER ld.bfd)
if(NOT CMAKE_LINKER)
	find_program(CMAKE_LINKER ld)
endif()
if(CMAKE_LINKER STREQUAL CMAKE_LINKER-NOTFOUND)
	message(FATAL_ERROR "C linker ld.bfd or ld not found - Please check your toolchain installation")
endif()
