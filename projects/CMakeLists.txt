# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED SERVICES_BINARY_DIR)
	message(FATAL_ERROR "A user error has occurred.
cmake was invoked with '${CMAKE_CURRENT_LIST_DIR}' specified as the source directory,
but it must be invoked with an application source directory,
such as '${CMAKE_CURRENT_LIST_DIR}/projects/hello_world'.
Debug variables:
CMAKE_CACHEFILE_DIR: ${CMAKE_CACHEFILE_DIR}
")
endif()

if(NOT DEFINED SERVICES_SOURCE_DIR)
	message(FATAL_ERROR "A user error has occurred.
cmake was invoked with '${CMAKE_CURRENT_LIST_DIR}' specified as the source directory,
but it must be invoked with an application source directory,
such as '${CMAKE_CURRENT_LIST_DIR}/projects/hello_world'.
Debug variables:
CMAKE_CACHEFILE_DIR: ${CMAKE_CACHEFILE_DIR}
")
endif()

# See https://gitlab.kitware.com/cmake/cmake/issues/16228
# and https://cmake.org/pipermail/cmake/2019-May/thread.html#69496
if(NOT ${MINIX_RT_BASE}/projects STREQUAL CMAKE_CURRENT_SOURCE_DIR)
message(WARNING "projects doesn't match CMAKE_CURRENT_SOURCE_DIR
	MINIX_RT_BASE              = ${MINIX_RT_BASE}
	PWD                      = $ENV{PWD}
	CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}
You may be using a mix of symbolic links and real paths which causes \
subtle and hard to debug CMake issues.")
endif()

# Verify that the toolchain can compile a dummy file, if it is not we
# won't be able to test for compatibility with certain C flags.
minix_rt_check_compiler_flag(C "" toolchain_is_ok)
assert(toolchain_is_ok "The toolchain is unable to build a dummy C file. See CMakeError.log.")

project(rootServices VERSION ${MINIX_RT_PROJECT_VERSION})
enable_language(C CXX ASM)

add_library(services_interface INTERFACE)

services_include_directories(
	${APPLICATION_BINARY_DIR}/include
)

services_compile_definitions(
	__SERVICES__
)

# Common toolchain-agnostic assembly flags
services_compile_options(
	$<$<COMPILE_LANGUAGE:ASM>:-D__ASSEMBLY__>
)

if(BUILD_VERSION)
	services_compile_definitions(
		BUILD_VERSION=${BUILD_VERSION}
	)
endif()

# @Intent: Add the basic toolchain warning flags
services_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,app_warning_base>>)
services_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,app_warning_base>>)

services_compile_options($<$<COMPILE_LANGUAGE:C>:-fno-strict-aliasing>)

get_property(OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG TARGET compiler PROPERTY no_optimization)
get_property(OPTIMIZE_FOR_DEBUG_FLAG TARGET compiler PROPERTY optimization_debug)
get_property(OPTIMIZE_FOR_SPEED_FLAG TARGET compiler PROPERTY optimization_speed)
get_property(OPTIMIZE_FOR_SIZE_FLAG  TARGET compiler PROPERTY optimization_size)

# From kconfig choice, pick the actual OPTIMIZATION_FLAG to use.
# Kconfig choice ensures only one of these CONFIG_*_OPTIMIZATIONS is set.
if(CONFIG_NO_OPTIMIZATIONS)
	set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_NO_OPTIMIZATIONS_FLAG})
elseif(CONFIG_DEBUG_OPTIMIZATIONS)
	set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_DEBUG_FLAG})
elseif(CONFIG_SPEED_OPTIMIZATIONS)
	set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SPEED_FLAG})
elseif(CONFIG_SIZE_OPTIMIZATIONS)
	set(OPTIMIZATION_FLAG ${OPTIMIZE_FOR_SIZE_FLAG}) # Default in kconfig
else()
	assert(0 "Unreachable code. Expected optimization level to have been chosen. See projects/Kconfig")
endif()

services_compile_options(${OPTIMIZATION_FLAG})

get_property(OPTIMIZE_DEBUG  TARGET compiler PROPERTY debug)

services_compile_options_ifdef(CONFIG_DEBUG_APP ${OPTIMIZE_DEBUG})

# @Intent: Obtain compiler specific flags for compiling under different ISO standards of C++
if(CONFIG_CPLUSPLUS)
	# From kconfig choice, pick a single dialect.
	# Kconfig choice ensures only one of these CONFIG_STD_CPP* is set.
	if(CONFIG_STD_CPP98)
		set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp98>)
	elseif(CONFIG_STD_CPP11)
		set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp11>) # Default in kconfig
	elseif(CONFIG_STD_CPP14)
		set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp14>)
	elseif(CONFIG_STD_CPP17)
		set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp17>)
	elseif(CONFIG_STD_CPP2A)
		set(STD_CPP_DIALECT_FLAGS $<TARGET_PROPERTY:compiler-cpp,dialect_cpp2a>)
	else()
		assert(0 "Unreachable code. Expected C++ standard to have been chosen.")
	endif()

	services_compile_options($<$<COMPILE_LANGUAGE:CXX>:${STD_CPP_DIALECT_FLAGS}>)
endif()

services_compile_options("SHELL: $<TARGET_PROPERTY:compiler,imacros> ${KCONFIG_H}")

# @Intent: Set compiler specific flag for bare metal freestanding option
services_compile_options($<TARGET_PROPERTY:compiler,freestanding>)

services_compile_options($<TARGET_PROPERTY:compiler,diagnostic>)

services_compile_options($<TARGET_PROPERTY:compiler,nostdinc>)
target_include_directories(services_interface SYSTEM INTERFACE $<TARGET_PROPERTY:compiler,nostdinc_include>)

services_ld_options(-nostdlib -static)

add_subdirectory(minix_rtruntime)

set_property(GLOBAL PROPERTY SERVICES_ARCHIVE_FILE "${SERVICES_BINARY_DIR}/archive.S")

function(minix_rtruntime_configure rootservicesname)
	get_property(SERVICES_LINKER_SCRIPT GLOBAL PROPERTY SERVICES_LINKER_SCRIPT_S)
	if(NOT EXISTS ${SERVICES_LINKER_SCRIPT})
		message(FATAL_ERROR "Could not find minix_rt linker script: '${CMAKE_CURRENT_SOURCE_DIR}'. ${CMAKE_CURRENT_LIST_FILE} Corrupted configuration?")
	endif()

	find_package(base REQUIRED)
	base_user_import_libraries()

	services_get_raw_include_directories_for_lang(base C SERVICES_IMPORTED_INCLUDE_DIRS)
	services_get_include_directories_for_lang(C SERVICES_INCLUDE_DIRS STRIP_PREFIX)

	configure_services_linker_script(
		${SERVICES_BINARY_DIR}/linker.lds
		"-D__ASSEMBLY__;-DLINKER_SCRIPT;-DAPP_LINKER_SCRIPT;-U${ARCH};${SERVICES_IMPORTED_INCLUDE_DIRS}"
	)

	add_custom_target(
		linker_services_prebuilt_script_target
		DEPENDS
		${SERVICES_BINARY_DIR}/linker.lds
	)

	set_property(TARGET
		linker_services_prebuilt_script_target
		PROPERTY INCLUDE_DIRECTORIES
		${SERVICES_INCLUDE_DIRS}
	)
endfunction()

function(DeclareRootservices rootservicesname)
	target_link_libraries(${rootservicesname} PRIVATE minix_rtruntime_crt_obj)

	minix_rtruntime_configure(${rootservicesname})
    set_property(
        TARGET ${rootservicesname}
        APPEND_STRING
        PROPERTY LINK_OPTIONS -Wl,-T ${SERVICES_BINARY_DIR}/linker.lds
    )
	set_property(
		TARGET
		${rootservicesname}
		PROPERTY LINK_DEPENDS
		${SERVICES_BINARY_DIR}/linker.lds
	)
	add_dependencies(${rootservicesname} linker_services_prebuilt_script_target)

	target_link_libraries(${rootservicesname} PUBLIC services_interface)

	get_property(SERVICES_ARCHIVE_FILE_PROPERTY GLOBAL PROPERTY SERVICES_ARCHIVE_FILE)
	make_kernel_archive(${SERVICES_ARCHIVE_FILE_PROPERTY} "$<TARGET_FILE:${rootservicesname}>")

	add_subdirectory(${KERNEL_SOURCE_DIR} ${kernel__build_dir})

	qemu_virt_setup()
endfunction(DeclareRootservices)

function(DeclareServicesApp servicesappname)
	target_link_libraries(${servicesappname} PRIVATE minix_rtruntime_crt_obj)

	minix_rtruntime_configure(${servicesappname})
    set_property(
        TARGET ${servicesappname}
        APPEND_STRING
        PROPERTY LINK_OPTIONS -Wl,-T ${SERVICES_BINARY_DIR}/linker.lds
    )
	set_property(
		TARGET
		${servicesappname}
		PROPERTY LINK_DEPENDS
		${SERVICES_BINARY_DIR}/linker.lds
	)
	add_dependencies(${servicesappname} linker_services_prebuilt_script_target)

	target_link_libraries(${servicesappname} PUBLIC services_interface)
endfunction(DeclareServicesApp)

function(make_kernel_archive output_name input_files)
	add_custom_command(
		OUTPUT ${output_name}
		COMMAND rm -f ${output_name}
		COMMAND ${SHELL} -c
		"echo 'X.section .archive_services,\"aw\"X.globl __archive_services_start, \
		__archive_services_endX__archive_services_start:X.incbin \"${input_files}\"X__archive_services_end:X' | tr X '\\n'"
		> ${output_name}
		DEPENDS
		${input_files}
		VERBATIM
	)
	add_custom_target(services_archive DEPENDS ${output_name})
endfunction()

function(qemu_virt_setup)
	if (CONFIG_QEMU_VIRT)
	get_property(CORE_IMAGE_NAME_PROPERTY GLOBAL PROPERTY CORE_IMAGE_NAME)
	get_property(CORE_IMAGE_TARGET_PROPERTY GLOBAL PROPERTY CORE_IMAGE_TARGET)
	add_custom_command(
		OUTPUT ${APPLICATION_BINARY_DIR}/simulate
		COMMAND ${PYTHON_EXECUTABLE} ${MINIX_RT_BASE}/scripts/gen_simulate.py
		-n ${CORE_IMAGE_NAME_PROPERTY}
		-o ${APPLICATION_BINARY_DIR}/simulate
		COMMAND chmod u+x "${APPLICATION_BINARY_DIR}/simulate"
		DEPENDS
		${CORE_IMAGE_TARGET_PROPERTY}
		WORKING_DIRECTORY
		${APPLICATION_BINARY_DIR}
	)
	add_custom_target(qemu_virt ALL DEPENDS ${APPLICATION_BINARY_DIR}/simulate)
	endif()
endfunction()
