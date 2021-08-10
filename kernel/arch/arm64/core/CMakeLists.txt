# SPDX-License-Identifier: Apache-2.0

kernel_library()

sel4m_library_sources(
	test1.c
	test2.S
	test3.c
)

sel4m_library_sources_ifndef(CONFIG_AAA test4.c)

set_property(GLOBAL PROPERTY KERNEL_HEAD_S "${CMAKE_CURRENT_LIST_DIR}/head.S")
