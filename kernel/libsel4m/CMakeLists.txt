# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${libsel4m_VAR}libsel4m VERSION 0.0.1)

add_library(${libsel4m_VAR}libsel4m STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)
add_subdirectory(arch/${ARCH}/src)

target_include_directories(${libsel4m_VAR}libsel4m PUBLIC
	include
	arch/${ARCH}/include
	../include/uapi
	../arch/${ARCH}/include/uapi
)

find_package(base REQUIRED)
base_user_import_libraries()

target_link_libraries(${libsel4m_VAR}libsel4m
	PUBLIC
	${libsel4m_VAR}base
)
