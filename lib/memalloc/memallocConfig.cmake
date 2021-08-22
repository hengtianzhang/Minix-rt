# SPDX-License-Identifier: Apache-2.0

set(MEMALLOC_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(MEMALLOC_LIBS_DIR)

function(memalloc_kernel_import_libraries)
	get_property(IMPORTED_MEMALLOC_PROERTY GLOBAL PROPERTY IMPORTED_MEMALLOC)
	if (NOT DEFINED IMPORTED_MEMALLOC_PROERTY)
		set_property(GLOBAL PROPERTY IMPORTED_MEMALLOC TRUE)

		add_subdirectory(${MEMALLOC_LIBS_DIR} memalloc)
	endif()

	get_property(KERNEL_IMPORTED_MEMALLOC_PROERTY GLOBAL PROPERTY KERNEL_IMPORTED_MEMALLOC)
	if (NOT DEFINED KERNEL_IMPORTED_MEMALLOC_PROERTY)
		set_property(GLOBAL PROPERTY KERNEL_IMPORTED_MEMALLOC TRUE)

		set_property(GLOBAL APPEND PROPERTY KERNEL_IMPORTED_LIBS memalloc)
        target_link_libraries(memalloc PUBLIC kernel_interface)
	endif()
endfunction()

function(memalloc_import_libraries)
	get_property(IMPORTED_MEMALLOC_PROERTY GLOBAL PROPERTY IMPORTED_MEMALLOC)
	if(NOT DEFINED IMPORTED_MEMALLOC_PROERTY)
		set_property(GLOBAL PROPERTY IMPORTED_MEMALLOC TRUE)

		add_subdirectory(${MEMALLOC_LIBS_DIR} memalloc)
	endif()
endfunction()
