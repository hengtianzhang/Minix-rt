
get_property(KERNEL_USE_DTB_PROPERTY GLOBAL PROPERTY KERNEL_USE_DTB)

if(DEFINED KERNEL_USE_DTB_PROPERTY)
	add_custom_command(
		OUTPUT
			${APPLICATION_BINARY_DIR}/include/generated/gen_dtb.h
		COMMAND
			${PYTHON_EXECUTABLE} ${SEL4M_BASE}/scripts/gen_dtb_h.py
				-i ${KERNEL_USE_DTB_PROPERTY}
				-o ${APPLICATION_BINARY_DIR}/include/generated/gen_dtb.h
		DEPENDS
		${KERNEL_USE_DTB_PROPERTY}
		WORKING_DIRECTORY
			${KERNEL_BINARY_DIR}
	)
	add_custom_target(gen_dtb_h DEPENDS ${APPLICATION_BINARY_DIR}/include/generated/gen_dtb.h)
	add_dependencies(kernel_generated_headers gen_dtb_h)
else()
	file(REMOVE ${APPLICATION_BINARY_DIR}/include/generated/gen_dtb.h)
	file(TOUCH ${APPLICATION_BINARY_DIR}/include/generated/gen_dtb.h)
endif()

find_package(of REQUIRED)
of_kernel_import_libraries()
