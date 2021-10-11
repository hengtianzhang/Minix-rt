# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${cpio_VAR}cpio VERSION 0.0.1)

add_library(${cpio_VAR}cpio STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)

target_include_directories(${cpio_VAR}cpio PUBLIC
	include
)
