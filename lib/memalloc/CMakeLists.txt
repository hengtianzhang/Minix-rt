# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(libmemalloc VERSION 0.0.1)

add_library(memalloc STATIC EXCLUDE_FROM_ALL)

target_sources(memalloc
	PRIVATE
	src/memblock.c
)

target_include_directories(memalloc PUBLIC include)
