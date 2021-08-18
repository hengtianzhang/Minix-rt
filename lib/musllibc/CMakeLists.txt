# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.13.1)

project(libmusllibc VERSION 0.0.1)

add_library(musllibc STATIC EXCLUDE_FROM_ALL)

target_sources(musllibc
	PRIVATE
	src/muslc.c
)

target_include_directories(musllibc PUBLIC include)
