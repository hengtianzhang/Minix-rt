# SPDX-License-Identifier: Apache-2.0

# This file must be included into the toplevel CMakeLists.txt file of
# sel4m applications.
# sel4m CMake package automatically includes this file when CMake function
# find_package() is used.
#
# To ensure this file is loaded in a sel4m application it must start with
# one of those lines:
#
# find_package(sel4m)
# find_package(sel4m REQUIRED HINTS $ENV{SEL4M_BASE})
#
# The `REQUIRED HINTS $ENV{SEL4M_BASE}` variant is required for any application
# inside the sel4m repository.
#
# It exists to reduce boilerplate code that sel4m expects to be in
# application CMakeLists.txt code.

# CMake version 3.13.1 is the real minimum supported version.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to
# define the required version, not even invoking it from an included
# file, like boilerplate.cmake, is sufficient. It is however permitted
# to have multiple invocations of cmake_minimum_required.
#
# Under these restraints we use a second 'cmake_minimum_required'
# invocation in every toplevel CMakeLists.txt.
cmake_minimum_required(VERSION 3.13.1)

# CMP0002: "Logical target names must be globally unique"
cmake_policy(SET CMP0002 NEW)

# Use the old CMake behaviour until the build scripts have been ported
# to the new behaviour.
# CMP0079: "target_link_libraries() allows use with targets in other directories"
cmake_policy(SET CMP0079 OLD)

define_property(GLOBAL PROPERTY KERNEL_LIBS
    BRIEF_DOCS "Global list of all kernel CMake libs that should be linked in"
    FULL_DOCS  "Global list of all kernel CMake libs that should be linked in.
kernel_library() appends libs to this list.")
set_property(GLOBAL PROPERTY KERNEL_LIBS "")

define_property(GLOBAL PROPERTY ELFLOADER_LIBS
    BRIEF_DOCS "Global list of all elfloader CMake libs that should be linked in"
    FULL_DOCS  "Global list of all elfloader CMake libs that should be linked in.
elfloader_library() appends libs to this list.")
set_property(GLOBAL PROPERTY ELFLOADER_LIBS "")

define_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES
  BRIEF_DOCS "Source files that are generated after kernel has been linked once."
  FULL_DOCS "\
Source files that are generated after kernel has been linked once.\
May include isr_tables.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_KERNEL_SOURCE_FILES "")

define_property(GLOBAL PROPERTY GENERATED_ELFLOADER_SOURCE_FILES
  BRIEF_DOCS "Source files that are generated after elfloader has been linked once."
  FULL_DOCS "\
Source files that are generated after elfloader has been linked once.\
May include isr_tables.c etc."
  )
set_property(GLOBAL PROPERTY GENERATED_ELFLOADER_SOURCE_FILES "")

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(elfloader__build_dir ${CMAKE_CURRENT_BINARY_DIR}/elfloader)
set(kernel__build_dir ${CMAKE_CURRENT_BINARY_DIR}/kernel)

set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

if(${CMAKE_VERSION} VERSION_EQUAL 3.19.0 OR
   ${CMAKE_VERSION} VERSION_EQUAL 3.19.1)
  message(WARNING "CMake 3.19.0/3.19.1 contains a bug regarding Toolchain/compiler "
          "testing. Consider switching to a different CMake version.")
  # This is a workaround for #30232.
  # During sel4m CMake invocation a plain C compiler is used for DTS.
  # This results in the internal `CheckCompilerFlag.cmake` being included by CMake
  # Later, when the full toolchain is configured, then `CMakeCheckCompilerFlag.cmake` is included.
  # This overloads the `cmake_check_compiler_flag()` function, thus causing #30232.
  # By manualy loading `CMakeCheckCompilerFlag.cmake` then `CheckCompilerFlag.cmake` will overload
  # the functions (and thus win the battle), and because `include_guard(GLOBAL)` is used in
  # `CMakeCheckCompilerFlag.cmake` this file will not be re-included later.
  include(${CMAKE_ROOT}/Modules/Internal/CMakeCheckCompilerFlag.cmake)
endif()

message(STATUS "Application: ${APPLICATION_SOURCE_DIR}")

# CMake's 'project' concept has proven to not be very useful for sel4m
# due in part to how sel4m is organized and in part to it not fitting well
# with cross compilation.
# sel4m therefore tries to rely as little as possible on project()
# and its associated variables, e.g. PROJECT_SOURCE_DIR.
# It is recommended to always use SEL4M_BASE instead of PROJECT_SOURCE_DIR
# when trying to reference ENV${SEL4M_BASE}.
set(ENV_SEL4M_BASE $ENV{SEL4M_BASE})
# This add support for old style boilerplate include.
if((NOT DEFINED SEL4M_BASE) AND (DEFINED ENV_SEL4M_BASE))
  set(SEL4M_BASE ${ENV_SEL4M_BASE} CACHE PATH "sel4m base")
endif()

# Note any later project() resets PROJECT_SOURCE_DIR
file(TO_CMAKE_PATH "${SEL4M_BASE}" PROJECT_SOURCE_DIR)

set(KERNEL_BINARY_DIR ${kernel__build_dir})
set(ELFLOADER_BINARY_DIR ${elfloader__build_dir})

set(AUTOCONF_H ${PROJECT_BINARY_DIR}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})

#
# Import more CMake functions and macros
#
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(${SEL4M_BASE}/cmake/extensions.cmake)
include(${SEL4M_BASE}/cmake/git.cmake)
include(${SEL4M_BASE}/cmake/version.cmake)  # depends on hex.cmake

#
# Find tools
#
include(${SEL4M_BASE}/cmake/python.cmake)
include(${SEL4M_BASE}/cmake/ccache.cmake)

set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/Kconfig)
file(MAKE_DIRECTORY ${KCONFIG_BINARY_DIR})

if(${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR})
  message(FATAL_ERROR "Source directory equals build directory.\
 In-source builds are not supported.\
 Please specify a build directory, e.g. cmake -Bbuild -H.")
endif()

add_custom_target(
  pristine
  COMMAND ${CMAKE_COMMAND} -DBINARY_DIR=${APPLICATION_BINARY_DIR}
          -DSOURCE_DIR=${APPLICATION_SOURCE_DIR}
          -P ${SEL4M_BASE}/cmake/pristine.cmake
  # Equivalent to rm -rf build/*
  )

# Check that BOARD has been provided, and that it has not changed.
sel4m_check_cache(ARCH REQUIRED)
set(ARCH_MESSAGE "Arch: ${ARCH}")
message(STATUS "${ARCH_MESSAGE}")









message(" aaaa ${PROJECT_SOURCE_DIR} asdsad ${CONFIG}")
