# SPDX-License-Identifier: Apache-2.0

target_sources(misc
	PRIVATE
	bitmap.c
	find_bit.c
	div64.c
)

target_sources_ifdef(CONFIG_GENERIC_HWEIGHT
	misc
	PRIVATE
	hweight.c
)
