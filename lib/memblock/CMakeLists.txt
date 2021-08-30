# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${memblock_VAR}memblock VERSION 0.0.1)

add_library(${memblock_VAR}memblock STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)

target_include_directories(${memblock_VAR}memblock PUBLIC
	include
)

find_package(base REQUIRED)
base_kernel_import_libraries()
base_user_import_libraries()

target_link_libraries(${memblock_VAR}memblock
	PUBLIC
	${memblock_VAR}base
)
