# SPDX-License-Identifier: Apache-2.0

file(MAKE_DIRECTORY ${APPLICATION_BINARY_DIR}/include/generated)

if(CONFIG_QEMU_VIRT)
	file(MAKE_DIRECTORY ${APPLICATION_BINARY_DIR}/dts)
	set(QEMU_OUTPUT_DTB ${APPLICATION_BINARY_DIR}/dts/qemu_virt.dtb)
	set_property(GLOBAL PROPERTY KERNEL_USE_DTB "${APPLICATION_BINARY_DIR}/dts/qemu_virt.dtb")
	execute_process(
		COMMAND
		qemu-system-${CONFIG_QEMU_SYSTEM} -cpu ${CONFIG_QEMU_CPU}
		-machine type=virt,${CONFIG_QEMU_VIRT_OPTION},dumpdtb=${QEMU_OUTPUT_DTB}
		-smp ${CONFIG_NR_CPUS} -m ${CONFIG_QEMU_MEMORY}
		-nographic
		OUTPUT_FILE ${QEMU_OUTPUT_DTB}
	)

	set(USE_QEMU_DTB 1)
endif()

if(CONFIG_DTC_OVERLAY_FILE AND (NOT DEFINED USE_QEMU_DTB))
	unset(DTC_OVERLAY_FILE_AS_LIST)
	unset(dts_files)
	# Convert from space-separated files into file list
	string(REPLACE " " ";" DTC_OVERLAY_FILE_RAW_LIST "${CONFIG_DTC_OVERLAY_FILE}")
	foreach(file ${DTC_OVERLAY_FILE_RAW_LIST})
		if (EXISTS ${APPLICATION_SOURCE_DIR}/dts/${file}.dts)
			list(APPEND DTC_OVERLAY_FILE_AS_LIST ${APPLICATION_SOURCE_DIR}/dts/${file}.dts)
		elseif(EXISTS ${SEL4M_BASE}/kernel/arch/${ARCH}/boot/dts/${file}.dts)
			list(APPEND DTC_OVERLAY_FILE_AS_LIST ${SEL4M_BASE}/kernel/arch/${ARCH}/boot/dts/${file}.dts)
		else()
			message(WARNING "Not found dts file: ${file}.dts")
		endif()
	endforeach()
	list(APPEND
		dts_files
		${DTC_OVERLAY_FILE_AS_LIST}
		)
	unset(DTC_OVERLAY_FILE_AS_LIST)

	if(NOT DEFINED CMAKE_DTS_PREPROCESSOR)
		set(CMAKE_DTS_PREPROCESSOR ${CMAKE_C_COMPILER})
	endif()

	sel4m_file(APPLICATION_ROOT DTS_ROOT)
	list(APPEND
		DTS_ROOT
  		${APPLICATION_SOURCE_DIR}
  		${SEL4M_BASE}/kernel/arch/${ARCH}/boot/dts
  	)
	list(REMOVE_DUPLICATES
		DTS_ROOT
	)

	file(MAKE_DIRECTORY ${APPLICATION_BINARY_DIR}/dts)
	set(i 0)
	foreach(file ${dts_files})
		message(STATUS "Found dts: ${file}")

		unset(DTS_ROOT_SYSTEM_INCLUDE_DIRS_${i})
		unset(DTS_ROOT_INCLUDE_DIRS_${i})
		foreach(dts_root ${DTS_ROOT})
			foreach(dts_root_path
				include
				dts/common
				dts/${ARCH}
				dts
			)
				get_filename_component(full_path ${dts_root}/${dts_root_path} REALPATH)
				if(EXISTS ${full_path})
					list(APPEND
						DTS_ROOT_SYSTEM_INCLUDE_DIRS_${i}
						-isystem ${full_path}
					)
					list(APPEND
						DTS_ROOT_INCLUDE_DIRS_${i}
						-i ${full_path}
					)
				endif()
			endforeach()
		endforeach()

		get_filename_component(dtsfile ${file} NAME_WE)
		execute_process(
			COMMAND ${CMAKE_DTS_PREPROCESSOR}
			-E   # Stop after preprocessing
			-P
			-Wp,-MD,${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.d.pre.tmp
			-nostdinc
			${DTS_ROOT_SYSTEM_INCLUDE_DIRS_${i}}
			-undef
			-D__DTS__
			-x assembler-with-cpp
			-o ${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.dts.tmp
			${file}
			WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
			RESULT_VARIABLE ret
			)
		if(NOT "${ret}" STREQUAL "0")
			message(FATAL_ERROR "command failed with return code: ${ret}")
		endif()

		if(DTC)
			execute_process(
				COMMAND ${DTC}
				-O dtb
				-o ${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb
				-b 0
				${DTS_ROOT_INCLUDE_DIRS_${i}}
				-Wno-unit_address_vs_reg
				-Wno-unit_address_format
				-Wno-avoid_unnecessary_addr_size
				-Wno-alias_paths
				-Wno-graph_child_address
				-Wno-simple_bus_reg
				-Wno-unique_unit_address
				-Wno-pci_device_reg
				-d ${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.d.dtc.tmp
				${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.dts.tmp
				OUTPUT_QUIET # Discard stdout
				WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
				RESULT_VARIABLE ret
			)
			if(NOT "${ret}" STREQUAL "0")
				message(FATAL_ERROR "command failed with return code: ${ret}")
			endif()

			set_property(GLOBAL PROPERTY KERNEL_USE_DTB "${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb")
		endif(DTC)

		execute_process(
			COMMAND cat
			${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.d.pre.tmp
			${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.d.dtc.tmp
			OUTPUT_FILE
			${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.d
			OUTPUT_QUIET # Discard stdout
			WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
			RESULT_VARIABLE ret
		)
		if(NOT "${ret}" STREQUAL "0")
			message(FATAL_ERROR "command failed with return code: ${ret}")
		endif()

		toolchain_parse_make_rule(
			${APPLICATION_BINARY_DIR}/dts/${dtsfile}.dtb.d
			include_files_${i} # Output parameter
			)

		set_property(DIRECTORY APPEND PROPERTY
			CMAKE_CONFIGURE_DEPENDS
			${include_files_${i}}
			)

		math(EXPR i "${i}+1")
	endforeach()

endif()
