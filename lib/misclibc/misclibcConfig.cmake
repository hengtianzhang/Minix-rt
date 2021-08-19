# SPDX-License-Identifier: Apache-2.0

set(MISCLIBC_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(MISCLIBC_LIBS_DIR)

function(misclibc_kernel_import_libraries)
	if(${KERNEL_IMPORTED_MISCLIBC})
		return()
	endif()

	set(KERNEL_IMPORTED_MISCLIBC TRUE )
	set(KERNEL_IMPORTED_MISCLIBC TRUE PARENT_SCOPE)
	mark_as_advanced(KERNEL_IMPORTED_MISCLIBC)

	add_subdirectory(${MISCLIBC_LIBS_DIR} misclibc)

	set_property(GLOBAL APPEND PROPERTY KERNEL_IMPORTED_LIBS misclibc)
endfunction()
