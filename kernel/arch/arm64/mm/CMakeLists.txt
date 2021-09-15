# SPDX-License-Identifier: Apache-2.0

kernel_library()

kernel_library_sources(
	cache.S proc.S
	mmu.c context.c init.c extable.c
	fault.c
)
