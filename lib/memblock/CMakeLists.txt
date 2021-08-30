# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${MEMBLOCK_VAR}memblock VERSION 0.0.1)

add_library(${MEMBLOCK_VAR}memblock STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)

target_include_directories(${MEMBLOCK_VAR}memblock PUBLIC
	include
)

find_package(base REQUIRED)
base_kernel_import_libraries()
base_user_import_libraries()

add_dependencies(${MEMBLOCK_VAR}memblock ${MEMBLOCK_VAR}base)
