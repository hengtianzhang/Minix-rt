# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(lib${BASE_VAR}base VERSION 0.0.1)

add_library(${BASE_VAR}base STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)

target_include_directories(${BASE_VAR}base PUBLIC
	include
)
