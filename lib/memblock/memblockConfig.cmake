# SPDX-License-Identifier: Apache-2.0

set(MEMBLOCK_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(MEMBLOCK_LIBS_DIR)

function(memblock_kernel_import_libraries)
	get_property(KERNEL_IMPORTED_MEMBLOCK_PROERTY GLOBAL PROPERTY KERNEL_IMPORTED_MEMBLOCK)
	if (NOT DEFINED KERNEL_IMPORTED_MEMBLOCK_PROERTY)
		set_property(GLOBAL PROPERTY KERNEL_IMPORTED_MEMBLOCK TRUE)
		set(MEMBLOCK_VAR "k")

		add_subdirectory(${MEMBLOCK_LIBS_DIR} kmemblock)

		set_property(GLOBAL APPEND PROPERTY KERNEL_IMPORTED_LIBS kmemblock)
		target_link_libraries(kmemblock PUBLIC kernel_interface)
	endif()
endfunction()

function(memblock_user_import_libraries)
	get_property(USER_IMPORTED_MEMBLOCK_PROERTY GLOBAL PROPERTY USER_IMPORTED_MEMBLOCK)
	if (NOT DEFINED USER_IMPORTED_MEMBLOCK_PROERTY)
		set_property(GLOBAL PROPERTY USER_IMPORTED_MEMBLOCK TRUE)
		set(MEMBLOCK_VAR "")

		add_subdirectory(${MEMBLOCK_LIBS_DIR} memblock)
		target_link_libraries(memblock PUBLIC services_interface)
	endif()
endfunction()
