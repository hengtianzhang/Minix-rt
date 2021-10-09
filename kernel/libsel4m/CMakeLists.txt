# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${libminix_rt_VAR}libminix_rt VERSION 0.0.1)

add_library(${libminix_rt_VAR}libminix_rt STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)
add_subdirectory(arch/${ARCH}/src)

target_include_directories(${libminix_rt_VAR}libminix_rt PUBLIC
	include
	arch/${ARCH}/include
	../include/uapi
	../arch/${ARCH}/include/uapi
)

find_package(base REQUIRED)
base_user_import_libraries()

target_link_libraries(${libminix_rt_VAR}libminix_rt
	PUBLIC
	${libminix_rt_VAR}base
)
