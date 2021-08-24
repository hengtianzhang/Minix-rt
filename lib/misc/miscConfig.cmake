# SPDX-License-Identifier: Apache-2.0

set(MISC_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(MISC_LIBS_DIR)

function(misc_kernel_import_libraries)
	get_property(IMPORTED_MISC_PROERTY GLOBAL PROPERTY IMPORTED_MISC)
	if (NOT DEFINED IMPORTED_MISC_PROERTY)
		set_property(GLOBAL PROPERTY IMPORTED_MISC TRUE)

		add_subdirectory(${MISC_LIBS_DIR} misc)
	endif()

	get_property(KERNEL_IMPORTED_MISC_PROERTY GLOBAL PROPERTY KERNEL_IMPORTED_MISC)
	if (NOT DEFINED KERNEL_IMPORTED_MISC)
		set_property(GLOBAL PROPERTY KERNEL_IMPORTED_MISC TRUE)

		set_property(GLOBAL APPEND PROPERTY KERNEL_IMPORTED_LIBS misc)
		target_link_libraries(misc PUBLIC kernel_interface)
	endif()
endfunction()

function(misc_import_libraries)
	get_property(IMPORTED_MISC_PROERTY GLOBAL PROPERTY IMPORTED_MISC)
	if(NOT DEFINED IMPORTED_MISC_PROERTY)
		set_property(GLOBAL PROPERTY IMPORTED_MISC TRUE)

		add_subdirectory(${MISC_LIBS_DIR} misc)
	endif()
endfunction()
