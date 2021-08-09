# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED KERNEL_BINARY_DIR)
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
if(NOT ${SEL4M_BASE}/kernel STREQUAL CMAKE_CURRENT_SOURCE_DIR)
message(WARNING "kernel doesn't match CMAKE_CURRENT_SOURCE_DIR
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

project(kernel VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

# In some cases the "final" things are not used at all and "_prebuilt"
# is the last station. See "logical_target_for_kernel_elf" below for
# details.
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(KERNEL_PREBUILT_EXECUTABLE kernel_prebuilt)
set(KERNEL_FINAL_EXECUTABLE    kernel_final)

define_property(GLOBAL PROPERTY PROPERTY_OUTPUT_FORMAT BRIEF_DOCS " " FULL_DOCS " ")

add_library(kernel_interface INTERFACE)

kernel_library_named(kernel)

kernel_include_directories(
	include
	arch/${ARCH}/include
	${APPLICATION_BINARY_DIR}/include/generated
)

kernel_compile_definitions(
	__KERNEL__
	__SEL4M__=1
)

kernel_compile_definitions($<TARGET_PROPERTY:compiler,security_fortify>)

if(BUILD_VERSION)
  kernel_compile_definitions(
    BUILD_VERSION=${BUILD_VERSION}
  )
endif()

# @Intent: Obtain compiler optimizations flags and store in variables
# @details:
#   If the user wants to tweak the optimizations, there are two ways:
#    1) Using EXTRA_CFLAGS which is applied regardless of kconfig choice, or
#    2) Rely on override support being implemented by your toolchain_cc_optimize_*()
#
get_property(OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG TARGET compiler PROPERTY no_optimization)
get_property(OPTIMIZE_FOR_DEBUG_FLAG TARGET compiler PROPERTY optimization_debug)
get_property(OPTIMIZE_FOR_SPEED_FLAG TARGET compiler PROPERTY optimization_speed)
get_property(OPTIMIZE_FOR_SIZE_FLAG  TARGET compiler PROPERTY optimization_size)

sel4m_library_sources(init/main.c)

