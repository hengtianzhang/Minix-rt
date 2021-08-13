# SPDX-License-Identifier: Apache-2.0

find_program(CMAKE_LINKER ld.lld)
if(CMAKE_LINKER STREQUAL CMAKE_LINKER-NOTFOUND)
	message(FATAL_ERROR "C linker ld.lld not found - Please check your toolchain installation")
endif()
