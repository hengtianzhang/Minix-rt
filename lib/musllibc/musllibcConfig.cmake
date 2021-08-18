# SPDX-License-Identifier: Apache-2.0

set(MUSLLIBC_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(MUSLLIBC_LIBS_DIR)

macro(musllibc_kernel_import_libraries)
	if(${KERNEL_IMPORTED_MUSLLIBC})
		return()
	endif()

	set(KERNEL_IMPORTED_MUSLLIBC TRUE PARENT_SCOPE)
	mark_as_advanced(KERNEL_IMPORTED_MUSLLIBC)

	add_subdirectory(${MUSLLIBC_LIBS_DIR} musllibc)

	set_property(GLOBAL APPEND PROPERTY KERNEL_IMPORTED_LIBS musllibc)
	target_link_libraries(musllibc kernel_interface)
endmacro()
