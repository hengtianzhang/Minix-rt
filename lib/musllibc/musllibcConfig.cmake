# SPDX-License-Identifier: Apache-2.0

set(MUSLLIBC_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(MUSLLIBC_LIBS_DIR)

function(musllibc_kernel_import_libraries)
	get_property(IMPORTED_MUSLLIBC_PROERTY GLOBAL PROPERTY IMPORTED_MUSLLIBC)
	if (NOT DEFINED IMPORTED_MUSLLIBC_PROERTY)
		set_property(GLOBAL PROPERTY IMPORTED_MUSLLIBC TRUE)

		add_subdirectory(${MUSLLIBC_LIBS_DIR} musllibc)
	endif()

	get_property(KERNEL_IMPORTED_MUSLLIBC_PROERTY GLOBAL PROPERTY KERNEL_IMPORTED_MUSLLIBC)
	if (NOT DEFINED KERNEL_IMPORTED_MUSLLIBC_PROERTY)
		set_property(GLOBAL PROPERTY KERNEL_IMPORTED_MUSLLIBC TRUE)

		set_property(GLOBAL APPEND PROPERTY KERNEL_IMPORTED_LIBS musllibc)
		target_link_libraries(musllibc PUBLIC kernel_interface)
	endif()
endfunction()

function(musllibc_import_libraries)
	get_property(IMPORTED_MUSLLIBC_PROERTY GLOBAL PROPERTY IMPORTED_MUSLLIBC)
	if (NOT DEFINED IMPORTED_MUSLLIBC_PROERTY)
		set_property(GLOBAL PROPERTY IMPORTED_MUSLLIBC TRUE)

		add_subdirectory(${MUSLLIBC_LIBS_DIR} musllibc)
	endif()
endfunction()
