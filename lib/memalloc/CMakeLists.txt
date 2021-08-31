# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${memalloc_VAR}memalloc VERSION 0.0.1)

add_library(${memalloc_VAR}memalloc STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)

target_include_directories(${memalloc_VAR}memalloc PUBLIC
	include
)

find_package(base REQUIRED)
base_kernel_import_libraries()
base_user_import_libraries()

target_link_libraries(${memalloc_VAR}memalloc
	PUBLIC
	${memalloc_VAR}base
)
