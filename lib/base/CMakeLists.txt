# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${base_VAR}base VERSION 0.0.1)

add_library(${base_VAR}base STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)
add_subdirectory(arch/${ARCH}/src)

target_include_directories(${base_VAR}base PUBLIC
	include
	arch/${ARCH}/include
)
