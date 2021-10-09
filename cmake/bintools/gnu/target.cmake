# SPDX-License-Identifier: Apache-2.0

# Configures binary tools as GNU binutils

if(DEFINED TOOLCHAIN_HOME)
	set_ifndef(find_program_binutils_args PATHS ${TOOLCHAIN_HOME})
endif()

find_program(CMAKE_OBJCOPY ${CROSS_COMPILE}objcopy ${find_program_gcc_args})
find_program(CMAKE_OBJDUMP ${CROSS_COMPILE}objdump ${find_program_gcc_args})
find_program(CMAKE_AS      ${CROSS_COMPILE}as      ${find_program_gcc_args})
find_program(CMAKE_AR      ${CROSS_COMPILE}ar      ${find_program_gcc_args})
find_program(CMAKE_RANLIB  ${CROSS_COMPILE}ranlib  ${find_program_gcc_args})
find_program(CMAKE_READELF ${CROSS_COMPILE}readelf ${find_program_gcc_args})
find_program(CMAKE_NM      ${CROSS_COMPILE}nm      ${find_program_gcc_args})
find_program(CMAKE_STRIP   ${CROSS_COMPILE}strip   ${find_program_gcc_args})

find_program(CMAKE_GDB     ${CROSS_COMPILE}gdb     ${find_program_gcc_args})
find_program(CMAKE_GDB     gdb-multiarch           ${find_program_binutils_args})

# Include bin tool properties
include(${MINIX_RT_BASE}/cmake/bintools/gnu/target_bintools.cmake)
