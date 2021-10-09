# SPDX-License-Identifier: Apache-2.0

unset(CMAKE_C_COMPILER)
unset(CMAKE_C_COMPILER CACHE)

unset(CMAKE_CXX_COMPILER)
unset(CMAKE_CXX_COMPILER CACHE)

unset(CMAKE_LINKER)
unset(CMAKE_LINKER CACHE)

if(NOT TOOLCHAIN_ROOT)
  if(DEFINED ENV{TOOLCHAIN_ROOT})
    # Support for out-of-tree toolchain
    set(TOOLCHAIN_ROOT $ENV{TOOLCHAIN_ROOT})
  else()
    # Default toolchain cmake file
    set(TOOLCHAIN_ROOT ${MINIX_RT_BASE})
  endif()
endif()
minix_rt_file(APPLICATION_ROOT TOOLCHAIN_ROOT)

# Don't inherit compiler flags from the environment
foreach(var AFLAGS CFLAGS CXXFLAGS CPPFLAGS LDFLAGS)
  if(DEFINED ENV{${var}})
    message(WARNING "The environment variable '${var}' was set to $ENV{${var}},
but minix_rt ignores flags from the environment. Use 'cmake -DEXTRA_${var}=$ENV{${var}}' instead.")
    unset(ENV{${var}})
  endif()
endforeach()

set(TOOLCHAIN_ROOT ${TOOLCHAIN_ROOT} CACHE STRING "minix_rt toolchain root" FORCE)
assert(TOOLCHAIN_ROOT "minix_rt toolchain root path invalid: please set the TOOLCHAIN_ROOT-variable")

# Set cached MINIX_RT_TOOLCHAIN.
set(MINIX_RT_TOOLCHAIN ${MINIX_RT_TOOLCHAIN} CACHE STRING "minix_rt toolchain variant")

# Configure the toolchain based on what SDK/toolchain is in use.
include(${TOOLCHAIN_ROOT}/cmake/toolchain/${MINIX_RT_TOOLCHAIN}/generic.cmake)

set_ifndef(TOOLCHAIN_KCONFIG_DIR ${TOOLCHAIN_ROOT}/cmake/toolchain/${MINIX_RT_TOOLCHAIN})

# Configure the toolchain based on what toolchain technology is used
# (gcc, host-gcc etc.)
include(${TOOLCHAIN_ROOT}/cmake/compiler/${COMPILER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/linker/${LINKER}/generic.cmake OPTIONAL)
include(${TOOLCHAIN_ROOT}/cmake/bintools/${BINTOOLS}/generic.cmake OPTIONAL)
