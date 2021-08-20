# SPDX-License-Identifier: Apache-2.0

get_property(elfconvert_formats TARGET bintools PROPERTY elfconvert_formats)
if(binary IN_LIST elfconvert_formats)
	set_property(GLOBAL APPEND
		PROPERTY KERNEL_POST_BUILD_COMMANDS
		COMMAND $<TARGET_PROPERTY:bintools,elfconvert_command>
			$<TARGET_PROPERTY:bintools,elfconvert_flag_outtarget>binary
			$<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.note
			$<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.note.gnu.build-id
			$<TARGET_PROPERTY:bintools,elfconvert_flag_section_remove>.comment
			$<TARGET_PROPERTY:bintools,elfconvert_flag_strip_all>
			$<TARGET_PROPERTY:bintools,elfconvert_flag_infile>${KERNEL_ELF_NAME}
			$<TARGET_PROPERTY:bintools,elfconvert_flag_outfile>${KERNEL_BIN_NAME}
			$<TARGET_PROPERTY:bintools,elfconvert_flag_final>
	)
	set_property(GLOBAL APPEND
	PROPERTY KERNEL_POST_BUILD_BYPRODUCTS
	${KERNEL_BIN_NAME}
	# ${out_hex_byprod} # Is this needed ?
	)

	set_property(GLOBAL APPEND
		PROPERTY KERNEL_POST_BUILD_COMMANDS
		COMMAND ${CMAKE_COMMAND}
		-E copy ${KERNEL_BIN_NAME} ${APPLICATION_BINARY_DIR}/images/Image
	)
	set_property(GLOBAL APPEND
	PROPERTY KERNEL_POST_BUILD_BYPRODUCTS
	${APPLICATION_BINARY_DIR}/images/Image
	)
	set_property(GLOBAL PROPERTY CORE_IMAGE_NAME "${APPLICATION_BINARY_DIR}/images/Image")
endif()
