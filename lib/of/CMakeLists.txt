# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${of_VAR}of VERSION 0.0.1)

add_library(${of_VAR}of STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)

target_include_directories(${of_VAR}of PUBLIC
	include
)

find_package(base REQUIRED)
base_kernel_import_libraries()
base_user_import_libraries()

target_link_libraries(${of_VAR}of
	PUBLIC
	${of_VAR}base
)
