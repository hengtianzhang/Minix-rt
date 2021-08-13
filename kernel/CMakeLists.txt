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
set(KERNEL_FINAL_EXECUTABLE    kernel_final)

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
set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SPEED_FLAG})

# @Intent: Obtain compiler specific flags related to C++ that are not influenced by kconfig
kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,required>>)

# @Intent: Obtain compiler specific flags for compiling under different ISO standards of C++
if(CONFIG_CPLUSPLUS)
  # From kconfig choice, pick a single dialect.
  # Kconfig choice ensures only one of these CONFIG_STD_CPP* is set.
  if(CONFIG_STD_CPP98)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp98>)
  elseif(CONFIG_STD_CPP11)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp11>) # Default in kconfig
  elseif(CONFIG_STD_CPP14)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp14>)
  elseif(CONFIG_STD_CPP17)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp17>)
  elseif(CONFIG_STD_CPP2A)
    set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp2a>)
  else()
    assert(0 "Unreachable code. Expected C++ standard to have been chosen.")
  endif()

  kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:${STD_CPP_DIALECT_FLAGS}>)
endif()

kernel_compile_options("SHELL: $<TARGET_PROPERTY:compiler,imacros> ${AUTOCONF_H}")

# @Intent: Set compiler specific flag for bare metal freestanding option
kernel_compile_options($<TARGET_PROPERTY:compiler,freestanding>)

# @Intent: Set compiler specific flag for tentative definitions, no-common
kernel_compile_options($<TARGET_PROPERTY:compiler,no_common>)

# @Intent: Obtain compiler specific flags related to assembly
# ToDo: Remember to get feedback from Oticon on this, as they might use the `ASM_BASE_FLAG` since this is done this way.
kernel_compile_options($<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,required>>)

# Common toolchain-agnostic assembly flags
kernel_compile_options(
  $<$<COMPILE_LANGUAGE:ASM>:-D__ASSEMBLY__>
)

# @Intent: Add the basic toolchain warning flags
kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_base>>)
kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_base>>)

# ==========================================================================
#
# cmake -DW=... settings
#
# W=1 - warnings that may be relevant and does not occur too often
# W=2 - warnings that occur quite often but may still be relevant
# W=3 - the more obscure warnings, can most likely be ignored
# ==========================================================================
# @Intent: Add cmake -DW toolchain supported warnings, if any
if(W MATCHES "1")
	kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_1>>)
	kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_1>>)
endif()

if(W MATCHES "2")
	kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_2>>)
	kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_2>>)
endif()

if(W MATCHES "3")
	kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_3>>)
	kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_3>>)
endif()

# @Intent: Add extended, more specific, toolchain warning flags
kernel_compile_options($<TARGET_PROPERTY:compiler,warning_extended>)

# @Intent: Trigger an error when a declaration does not specify a type
kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_error_implicit_int>>)
kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_error_implicit_int>>)

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(${SEL4M_BASE}/cmake/kernel_extra_flags.cmake)

kernel_cc_option(-fno-asynchronous-unwind-tables)
kernel_cc_option(-fno-pie)
kernel_cc_option(-fno-pic)
kernel_cc_option(-fno-strict-overflow)

kernel_cc_option(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=CMAKE_SOURCE_DIR)
kernel_cc_option(-fmacro-prefix-map=${SEL4M_BASE}=SEL4M_BASE)

kernel_compile_options($<TARGET_PROPERTY:compiler,nostdinc>)
target_include_directories(kernel_interface SYSTEM INTERFACE $<TARGET_PROPERTY:compiler,nostdinc_include>)

get_property(CSTD GLOBAL PROPERTY CSTD)
set_ifndef(CSTD gnu89)

# @Intent: Obtain compiler specific flag for specifying the c standard
kernel_compile_options(
  $<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,cstd>${CSTD}>
)

get_property(TOPT GLOBAL PROPERTY TOPT)

set_ifndef(  TOPT -Wl,-T) # clang doesn't pick -T for some reason and complains,
                          # while -Wl,-T works for both, gcc and clang

kernel_library_sources(init/main.c init/test.S)

set(LINKER_SCRIPT ${KERNEL_DIR}/arch/${ARCH}/core/linker.lds.S)

if(NOT EXISTS ${LINKER_SCRIPT})
  message(FATAL_ERROR "Could not find linker script: '${CMAKE_CURRENT_SOURCE_DIR}'. ${CMAKE_CURRENT_LIST_FILE} Corrupted configuration?")
endif()

add_subdirectory(arch/${ARCH})

get_property(HEAD_S GLOBAL PROPERTY KERNEL_HEAD_S)

if(NOT EXISTS ${HEAD_S})
	message("Could not find header file: HEAD_S")
endif()
