# SPDX-License-Identifier: Apache-2.0

kernel_library()

kernel_library_sources(
	test1.c
	test2.S
	test3.c
)

set_property(GLOBAL PROPERTY LINKER_SCRIPT_S "${CMAKE_CURRENT_LIST_DIR}/linker.lds.S")
set_property(GLOBAL PROPERTY KERNEL_HEAD_S "${CMAKE_CURRENT_LIST_DIR}/head.S")
