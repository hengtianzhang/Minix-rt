# SPDX-License-Identifier: Apache-2.0

# This file must be included into the toplevel CMakeLists.txt file of
# minix_rt applications.
# minix_rt CMake package automatically includes this file when CMake function
# find_package() is used.
#
# To ensure this file is loaded in a minix_rt application it must start with
# one of those lines:
#
# find_package(minix_rt)
# find_package(minix_rt REQUIRED HINTS $ENV{MINIX_RT_BASE})
#
# The `REQUIRED HINTS $ENV{MINIX_RT_BASE}` variant is required for any application
# inside the minix_rt repository.
#
# It exists to reduce boilerplate code that minix_rt expects to be in
# application CMakeLists.txt code.

# CMake version 3.20.0 is the real minimum supported version.
#
# Makefile Generators, for some toolchains, now use the compiler to extract 
# implicit dependencies while compiling source files.
#
# Unfortunately CMake requires the toplevel CMakeLists.txt file to
# define the required version, not even invoking it from an included
# file, like boilerplate.cmake, is sufficient. It is however permitted
# to have multiple invocations of cmake_minimum_required.
#
# Under these restraints we use a second 'cmake_minimum_required'
# invocation in every toplevel CMakeLists.txt.
cmake_minimum_required(VERSION 3.20.0)

# CMP0002: "Logical target names must be globally unique"
cmake_policy(SET CMP0002 NEW)

# CMP0079: "target_link_libraries() allows use with targets in other directories"
cmake_policy(SET CMP0079 NEW)

define_property(GLOBAL PROPERTY KERNEL_BUILT_IN_LIBS
    BRIEF_DOCS "Global list of all kernel built-in CMake libs that should be linked in"
    FULL_DOCS  "Global list of all kernel built-in CMake libs that should be linked in.
kernel_library() appends libs to this list.")
set_property(GLOBAL PROPERTY KERNEL_BUILT_IN_LIBS "")

define_property(GLOBAL PROPERTY KERNEL_INTERFACE_LIBS
    BRIEF_DOCS "Global list of all kernel interface CMake libs that should be linked in"
    FULL_DOCS  "Global list of all kernel interface CMake libs that should be linked in.
kernel_interface_library() appends libs to this list.")
set_property(GLOBAL PROPERTY KERNEL_INTERFACE_LIBS "")

define_property(GLOBAL PROPERTY KERNEL_IMPORTED_LIBS
    BRIEF_DOCS "Global list of all kernel imported CMake libs that should be linked in"
    FULL_DOCS  "Global list of all kernel imported CMake libs that should be linked in.
xxxx_kernel_import_libraries() appends libs to this list.")
set_property(GLOBAL PROPERTY KERNEL_IMPORTED_LIBS "")

set(APPLICATION_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR} CACHE PATH "Application Source Directory")
set(APPLICATION_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR} CACHE PATH "Application Binary Directory")

set(kernel__build_dir ${APPLICATION_BINARY_DIR}/kernel)
set(services__build_dir ${APPLICATION_BINARY_DIR}/projects)
set(drivers__build_dir ${APPLICATION_BINARY_DIR}/drivers)
set(servers__build_dir ${APPLICATION_BINARY_DIR}/servers)

set(kernel__sources_dir ${MINIX_RT_BASE}/kernel)
set(services__sources_dir ${MINIX_RT_BASE}/projects)
set(drivers__sources_dir ${MINIX_RT_BASE}/drivers)
set(servers__sources_dir ${MINIX_RT_BASE}/servers)

set(PROJECT_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})

message(STATUS "Application: ${APPLICATION_SOURCE_DIR}")

# CMake's 'project' concept has proven to not be very useful for minix_rt
# due in part to how minix_rt is organized and in part to it not fitting well
# with cross compilation.
# minix_rt therefore tries to rely as little as possible on project()
# and its associated variables, e.g. PROJECT_SOURCE_DIR.
# It is recommended to always use MINIX_RT_BASE instead of PROJECT_SOURCE_DIR
# when trying to reference ENV${MINIX_RT_BASE}.
set(ENV_MINIX_RT_BASE $ENV{MINIX_RT_BASE})
# This add support for old style boilerplate include.
if((NOT DEFINED MINIX_RT_BASE) AND (DEFINED ENV_MINIX_RT_BASE))
  set(MINIX_RT_BASE ${ENV_MINIX_RT_BASE} CACHE PATH "minix_rt base")
endif()

# Note any later project() resets PROJECT_SOURCE_DIR
file(TO_CMAKE_PATH "${MINIX_RT_BASE}" PROJECT_SOURCE_DIR)

set(KERNEL_BINARY_DIR ${kernel__build_dir})
set(SERVICES_BINARY_DIR ${services__build_dir})

set(KERNEL_SOURCE_DIR ${kernel__sources_dir})
set(SERVICES_SOURCE_DIR ${services__sources_dir})

set(AUTOCONF_H ${APPLICATION_BINARY_DIR}/include/generated/autoconf.h)
# Re-configure (Re-execute all CMakeLists.txt code) when autoconf.h changes
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${AUTOCONF_H})

#
# Import more CMake functions and macros
#
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(${MINIX_RT_BASE}/cmake/extensions.cmake)
include(${MINIX_RT_BASE}/cmake/git.cmake)
include(${MINIX_RT_BASE}/cmake/version.cmake)  # depends on hex.cmake

#
# Find tools
#
include(${MINIX_RT_BASE}/cmake/python.cmake)
include(${MINIX_RT_BASE}/cmake/ccache.cmake)
include(${MINIX_RT_BASE}/cmake/cpio.cmake)

set(KCONFIG_BINARY_DIR ${CMAKE_BINARY_DIR}/kconfig)
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
          -P ${MINIX_RT_BASE}/cmake/pristine.cmake
  # Equivalent to rm -rf build/*
  )

# Check that BOARD has been provided, and that it has not changed.
minix_rt_check_cache(ARCH REQUIRED)
set(ARCH_MESSAGE "Arch: ${ARCH}")
message(STATUS "${ARCH_MESSAGE}")

# Populate USER_CACHE_DIR with a directory that user applications may
# write cache files to.
if(NOT DEFINED USER_CACHE_DIR)
  find_appropriate_cache_directory(USER_CACHE_DIR)
endif()
message(STATUS "Cache files will be written to: ${USER_CACHE_DIR}")

# Prevent CMake from testing the toolchain
set(CMAKE_C_COMPILER_FORCED   1)
set(CMAKE_CXX_COMPILER_FORCED 1)

include(${MINIX_RT_BASE}/cmake/verify-toolchain.cmake)
include(${MINIX_RT_BASE}/cmake/host-tools.cmake)

# DTS should be close to kconfig because CONFIG_ variables from
# kconfig and dts should be available at the same time.
#
# The DT system uses a C preprocessor for it's code generation needs.
# This creates an awkward chicken-and-egg problem, because we don't
# always know exactly which toolchain the user needs until we know
# more about the target, e.g. after DT and Kconfig.
#
# To resolve this we find "some" C toolchain, configure it generically
# with the minimal amount of configuration needed to have it
# preprocess DT sources, and then, after we have finished processing
# both DT and Kconfig we complete the target-specific configuration,
# and possibly change the toolchain.
include(${MINIX_RT_BASE}/cmake/generic_toolchain.cmake)

# Here, Some host-tool is done.
# TODO

include(${MINIX_RT_BASE}/cmake/target_toolchain.cmake)

enable_language(C CXX ASM)
# The setup / configuration of the toolchain itself and the configuration of
# supported compilation flags are now split, as this allows to use the toolchain
# for generic purposes, for example DTS, and then test the toolchain for
# supported flags at stage two.
# Testing the toolchain flags requires the enable_language() to have been called in CMake.
include(${MINIX_RT_BASE}/cmake/target_toolchain_flags.cmake)

include(${MINIX_RT_BASE}/cmake/kconfig.cmake)

include(${MINIX_RT_BASE}/cmake/dts.cmake)

set(CMAKE_PREFIX_PATH ${MINIX_RT_BASE} CACHE PATH "")
list(APPEND CMAKE_PREFIX_PATH ${MINIX_RT_BASE}/kernel/libminix-rt)

configure_file(${MINIX_RT_BASE}/version.h.in ${APPLICATION_BINARY_DIR}/include/generated/version.h)

set(KERNEL_SOURCE_DIR ${MINIX_RT_BASE}/kernel CACHE PATH "")
set(SERVICES_SOURCE_DIR ${MINIX_RT_BASE}/projects CACHE PATH "")
set(DRIVERS_SOURCE_DIR ${MINIX_RT_BASE}/drivers CACHE PATH "")
set(SERVERS_SOURCE_DIR ${MINIX_RT_BASE}/servers CACHE PATH "")

add_subdirectory(${SERVICES_SOURCE_DIR}  ${services__build_dir})
add_subdirectory(${DRIVERS_SOURCE_DIR}   ${drivers__build_dir})
add_subdirectory(${SERVERS_SOURCE_DIR}   ${servers__build_dir})

get_property(SERVICES_ARCHIVE_FILE_PROPERTY GLOBAL PROPERTY SERVICES_ARCHIVE_FILE)
get_property(DRIVERS_ARCHIVE_FILE_PROPERTY GLOBAL PROPERTY DRIVERS_ARCHIVE_FILE)
get_property(SERVICES_ARCHIVE_FILE_LIST_PROPERTY GLOBAL PROPERTY SERVICES_ARCHIVE_FILE_LIST)
get_property(DRIVERS_ARCHIVE_FILE_LIST_PROPERTY GLOBAL PROPERTY DRIVERS_ARCHIVE_FILE_LIST)

make_kernel_drivers_archive(${DRIVERS_ARCHIVE_FILE_PROPERTY} ${DRIVERS_ARCHIVE_FILE_LIST_PROPERTY})
make_kernel_services_archive(${SERVICES_ARCHIVE_FILE_PROPERTY} ${SERVICES_ARCHIVE_FILE_LIST_PROPERTY})

add_subdirectory(${KERNEL_SOURCE_DIR}    ${kernel__build_dir})

qemu_virt_setup()
