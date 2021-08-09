# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED ELFLOADER_BINARY_DIR)
  message(FATAL_ERROR "A user error has occurred.
cmake was invoked with '${CMAKE_CURRENT_LIST_DIR}' specified as the source directory,
but it must be invoked with an application source directory,
such as '${CMAKE_CURRENT_LIST_DIR}/projects/hello_world'.
Debug variables:
CMAKE_CACHEFILE_DIR: ${CMAKE_CACHEFILE_DIR}
")
endif()

# See https://gitlab.kitware.com/cmake/cmake/issues/16228
# and https://cmake.org/pipermail/cmake/2019-May/thread.html#69496
if(NOT ${SEL4M_BASE}/elfloader STREQUAL CMAKE_CURRENT_SOURCE_DIR)
message(WARNING "elfloader doesn't match CMAKE_CURRENT_SOURCE_DIR
  SEL4M_BASE              = ${SEL4M_BASE}
  PWD                      = $ENV{PWD}
  CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}
You may be using a mix of symbolic links and real paths which causes \
subtle and hard to debug CMake issues.")
endif()

# Verify that the toolchain can compile a dummy file, if it is not we
# won't be able to test for compatibility with certain C flags.
sel4m_check_compiler_flag(C "" toolchain_is_ok)
assert(toolchain_is_ok "The toolchain is unable to build a dummy C file. See CMakeError.log.")

project(elfloader VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

