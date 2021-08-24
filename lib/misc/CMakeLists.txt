# SPDX-License-Identifier: Apache-2.0

cmake_minimum_required(VERSION 3.20.0)

project(libmisc VERSION 0.0.1)

add_library(misc STATIC EXCLUDE_FROM_ALL)

add_subdirectory(src)
add_subdirectory(arch/${ARCH})

# Common toolchain-agnostic assembly flags
target_compile_options(misc PUBLIC
	$<$<COMPILE_LANGUAGE:ASM>:-D__ASSEMBLY__>
)

target_include_directories(misc PUBLIC
	include
	arch/${ARCH}/include
)
