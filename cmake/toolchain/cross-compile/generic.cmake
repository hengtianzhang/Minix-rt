# SPDX-License-Identifier: Apache-2.0

# CROSS_COMPILE is a KBuild mechanism for specifying an external
# toolchain with a single environment variable.
#
# It is a legacy mechanism that will in minix_rt translate to
# specififying MINIX_RT_TOOLCHAIN to 'cross-compile' with the location
# 'CROSS_COMPILE'.
#
# It can be set from either the environment or from a CMake variable
# of the same name.
#
# The env var has the lowest precedence.

if((NOT (DEFINED CROSS_COMPILE)) AND (DEFINED ENV{CROSS_COMPILE}))
  set(CROSS_COMPILE $ENV{CROSS_COMPILE})
endif()

set(CROSS_COMPILE ${CROSS_COMPILE} CACHE STRING "")
assert(CROSS_COMPILE "CROSS_COMPILE is not set")

set(COMPILER gcc)
set(LINKER ld)
set(BINTOOLS gnu)

message(STATUS "Found toolchain: cross-compile (${CROSS_COMPILE})")
