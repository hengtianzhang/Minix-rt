# SPDX-License-Identifier: Apache-2.0

set(BASE_LIBS_DIR "${CMAKE_CURRENT_LIST_DIR}" CACHE STRING "")
mark_as_advanced(BASE_LIBS_DIR)

function(base_kernel_import_libraries)
	get_property(KERNEL_IMPORTED_BASE_PROERTY GLOBAL PROPERTY KERNEL_IMPORTED_BASE)
	if (NOT DEFINED KERNEL_IMPORTED_BASE_PROERTY)
		set_property(GLOBAL PROPERTY KERNEL_IMPORTED_BASE TRUE)
		set(BASE_VAR "k")

		add_subdirectory(${BASE_LIBS_DIR} kbase)

		set_property(GLOBAL APPEND PROPERTY KERNEL_IMPORTED_LIBS kbase)
		target_link_libraries(kbase PUBLIC kernel_interface)
	endif()
endfunction()

function(base_user_import_libraries)
	get_property(USER_IMPORTED_BASE_PROERTY GLOBAL PROPERTY USER_IMPORTED_BASE)
	if (NOT DEFINED USER_IMPORTED_BASE_PROERTY)
		set_property(GLOBAL PROPERTY USER_IMPORTED_BASE TRUE)
		set(BASE_VAR "")

		add_subdirectory(${BASE_LIBS_DIR} base)
		target_link_libraries(base PUBLIC services_interface)
	endif()
endfunction()
