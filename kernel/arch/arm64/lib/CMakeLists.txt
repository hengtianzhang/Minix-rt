# SPDX-License-Identifier: Apache-2.0

kernel_interface_library()

kernel_interface_library_sources(
	clear_page.S delay.c copy_from_user.S
	copy_to_user.S clear_user.S copy_page.S
)
