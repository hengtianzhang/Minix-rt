# SPDX-License-Identifier: Apache-2.0

# Folders needed for conf/mconf files (kconfig has no method of redirecting all output files).
# conf/mconf needs to be run from a different directory because of: GH-3408
file(MAKE_DIRECTORY ${APPLICATION_BINARY_DIR}/include/generated)
file(MAKE_DIRECTORY ${APPLICATION_BINARY_DIR}/include/config)

if(KCONFIG_ROOT)
	minix_rt_file(APPLICATION_ROOT KCONFIG_ROOT)
else()
	set(KCONFIG_ROOT ${MINIX_RT_BASE}/Kconfig)
endif()

set(DOTCONFIG                  ${APPLICATION_BINARY_DIR}/.config)
set(PARSED_KCONFIG_SOURCES_TXT ${APPLICATION_BINARY_DIR}/kconfig/sources.txt)

if(DEFINED KCONFIG_FILE)
	file(REMOVE ${DOTCONFIG})
	unset(MINIX_RT_DEFCONFIG)
	unset(MINIX_RT_DEFCONFIG CACHE)
endif()

find_file(MINIX_RT_DEFCONFIG ${KCONFIG_FILE} ${MINIX_RT_BASE}/kernel/arch/${ARCH}/configs)

if (MINIX_RT_DEFCONFIG STREQUAL MINIX_RT_DEFCONFIG-NOTFOUND)
	message(FATAL_ERROR "Not Found KCONFIG_FILE. The example:
	-DKCONFIG_FILE=defconfig
")
endif()

unset(KCONFIG_FILE)
unset(KCONFIG_FILE CACHE)

message(STATUS "Kconfig file: ${MINIX_RT_DEFCONFIG}")

get_filename_component(APPLICATION_SOURCE ${APPLICATION_SOURCE_DIR} NAME)
set(APPLICATION_SOURCE ${APPLICATION_SOURCE})

set(COMMON_KCONFIG_ENV_SETTINGS
	PYTHON_EXECUTABLE=${PYTHON_EXECUTABLE}
	srctree=${MINIX_RT_BASE}
	MINIX_RT_VERSION_STRING=${MINIX_RT_VERSION_STRING}
	KCONFIG_CONFIG=${DOTCONFIG}
	# Set environment variables so that Kconfig can prune Kconfig source
	# files for other architectures
	ARCH=${ARCH}
	KCONFIG_BINARY_DIR=${KCONFIG_BINARY_DIR}
	TOOLCHAIN_KCONFIG_DIR=${TOOLCHAIN_KCONFIG_DIR}
	APPLICATION_SOURCE=${APPLICATION_SOURCE}
	APPLICATION_SOURCE_DIR=${APPLICATION_SOURCE_DIR}
	CC=${CMAKE_C_COMPILER}
	LD=${CMAKE_LINKER}
)

# Allow out-of-tree users to add their own Kconfig python frontend
# targets by appending targets to the CMake list
# 'EXTRA_KCONFIG_TARGETS' and setting variables named
# 'EXTRA_KCONFIG_TARGET_COMMAND_FOR_<target>'
#
# e.g.
# cmake -DEXTRA_KCONFIG_TARGETS=cli
# -DEXTRA_KCONFIG_TARGET_COMMAND_FOR_cli=cli_kconfig_frontend.py

set(EXTRA_KCONFIG_TARGET_COMMAND_FOR_menuconfig
	${MINIX_RT_BASE}/scripts/kconfig/menuconfig.py
	)

set(EXTRA_KCONFIG_TARGET_COMMAND_FOR_guiconfig
	${MINIX_RT_BASE}/scripts/kconfig/guiconfig.py
	)

set(EXTRA_KCONFIG_TARGET_COMMAND_FOR_hardenconfig
	${MINIX_RT_BASE}/scripts/kconfig/hardenconfig.py
	)

foreach(kconfig_target
		menuconfig
		guiconfig
		hardenconfig
		${EXTRA_KCONFIG_TARGETS}
		)
	add_custom_target(
		${kconfig_target}
		${CMAKE_COMMAND} -E env
		MINIX_RT_BASE=${MINIX_RT_BASE}
		MINIX_RT_TOOLCHAIN=${MINIX_RT_TOOLCHAIN}
		${COMMON_KCONFIG_ENV_SETTINGS}
		${PYTHON_EXECUTABLE}
		${EXTRA_KCONFIG_TARGET_COMMAND_FOR_${kconfig_target}}
		${KCONFIG_ROOT}
		WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}/kconfig
		USES_TERMINAL
		COMMAND_EXPAND_LISTS
		)
endforeach()

# Support assigning Kconfig symbols on the command-line with CMake
# cache variables prefixed with 'CONFIG_'. This feature is
# experimental and undocumented until it has undergone more
# user-testing.
unset(EXTRA_KCONFIG_OPTIONS)
get_cmake_property(cache_variable_names CACHE_VARIABLES)
foreach (name ${cache_variable_names})
	if("${name}" MATCHES "^CONFIG_")
		# When a cache variable starts with 'CONFIG_', it is assumed to be
		# a Kconfig symbol assignment from the CMake command line.
		set(EXTRA_KCONFIG_OPTIONS
			"${EXTRA_KCONFIG_OPTIONS}\n${name}=${${name}}"
			)
	endif()
endforeach()

if(EXTRA_KCONFIG_OPTIONS)
	set(EXTRA_KCONFIG_OPTIONS_FILE ${APPLICATION_BINARY_DIR}/misc/generated/extra_kconfig_options.conf)
	file(WRITE
		${EXTRA_KCONFIG_OPTIONS_FILE}
		${EXTRA_KCONFIG_OPTIONS}
		)
endif()

# Bring in extra configuration files dropped in by the user or anyone else;
# make sure they are set at the end so we can override any other setting
file(GLOB config_files ${APPLICATION_BINARY_DIR}/*.conf)
list(SORT config_files)
set(
	merge_config_files
	${MINIX_RT_DEFCONFIG}
	${CONF_FILE_AS_LIST}
	${shield_conf_files}
	${OVERLAY_CONFIG_AS_LIST}
	${EXTRA_KCONFIG_OPTIONS_FILE}
	${config_files}
)

# Create a list of absolute paths to the .config sources from
# merge_config_files, which is a mix of absolute and relative paths.
set(merge_config_files_with_absolute_paths "")
foreach(f ${merge_config_files})
	if(IS_ABSOLUTE ${f})
		set(path ${f})
	else()
		set(path ${APPLICATION_SOURCE_DIR}/${f})
	endif()

	list(APPEND merge_config_files_with_absolute_paths ${path})
endforeach()

foreach(f ${merge_config_files_with_absolute_paths})
	if(NOT EXISTS ${f} OR IS_DIRECTORY ${f})
		message(FATAL_ERROR "File not found: ${f}")
	endif()
endforeach()

# Calculate a checksum of merge_config_files to determine if we need
# to re-generate .config
set(merge_config_files_checksum "")
foreach(f ${merge_config_files_with_absolute_paths})
	file(MD5 ${f} checksum)
	set(merge_config_files_checksum "${merge_config_files_checksum}${checksum}")
endforeach()

# Create a new .config if it does not exists, or if the checksum of
# the dependencies has changed
set(merge_config_files_checksum_file ${APPLICATION_BINARY_DIR}/.cmake.dotconfig.checksum)
set(CREATE_NEW_DOTCONFIG 1)
# Check if the checksum file exists too before trying to open it, though it
# should under normal circumstances
if(EXISTS ${DOTCONFIG} AND EXISTS ${merge_config_files_checksum_file})
	# Read out what the checksum was previously
	file(READ
		${merge_config_files_checksum_file}
		merge_config_files_checksum_prev
		)
	if(
			${merge_config_files_checksum} STREQUAL
			${merge_config_files_checksum_prev}
			)
		# Checksum is the same as before
		set(CREATE_NEW_DOTCONFIG 0)
	endif()
endif()

if(CREATE_NEW_DOTCONFIG)
	set(input_configs_are_handwritten --handwritten-input-configs)
	set(input_configs ${merge_config_files})
else()
	set(input_configs ${DOTCONFIG})
endif()

execute_process(
	COMMAND ${CMAKE_COMMAND} -E env
	${COMMON_KCONFIG_ENV_SETTINGS}
	SHIELD_AS_LIST=${SHIELD_AS_LIST_ESCAPED}
	${PYTHON_EXECUTABLE}
	${MINIX_RT_BASE}/scripts/kconfig/kconfig.py
	--minix_rt-base=${MINIX_RT_BASE}
	${input_configs_are_handwritten}
	${KCONFIG_ROOT}
	${DOTCONFIG}
	${AUTOCONF_H}
	${PARSED_KCONFIG_SOURCES_TXT}
	${input_configs}
	WORKING_DIRECTORY ${APPLICATION_SOURCE_DIR}
	# The working directory is set to the app dir such that the user
	# can use relative paths in CONF_FILE, e.g. CONF_FILE=nrf5.conf
	RESULT_VARIABLE ret
	)
if(NOT "${ret}" STREQUAL "0")
	message(FATAL_ERROR "command failed with return code: ${ret}")
endif()

if(CREATE_NEW_DOTCONFIG)
	# Write the new configuration fragment checksum. Only do this if kconfig.py
	# succeeds, to avoid marking minix_rt/.config as up-to-date when it hasn't been
	# regenerated.
	file(WRITE ${merge_config_files_checksum_file}
						 ${merge_config_files_checksum})
endif()

# Read out the list of 'Kconfig' sources that were used by the engine.
file(STRINGS ${PARSED_KCONFIG_SOURCES_TXT} PARSED_KCONFIG_SOURCES_LIST)

# Force CMAKE configure when the Kconfig sources or configuration files changes.
foreach(kconfig_input
		${merge_config_files}
		${DOTCONFIG}
		${PARSED_KCONFIG_SOURCES_LIST}
		)
	set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${kconfig_input})
endforeach()

add_custom_target(config-twister DEPENDS ${DOTCONFIG})

# Remove the CLI Kconfig symbols from the namespace and
# CMakeCache.txt. If the symbols end up in DOTCONFIG they will be
# re-introduced to the namespace through 'import_kconfig'.
foreach (name ${cache_variable_names})
	if("${name}" MATCHES "^CONFIG_")
		unset(${name})
		unset(${name} CACHE)
	endif()
endforeach()

# Parse the lines prefixed with CONFIG_ in the .config file from Kconfig
import_kconfig(CONFIG_ ${DOTCONFIG})

# Re-introduce the CLI Kconfig symbols that survived
foreach (name ${cache_variable_names})
	if("${name}" MATCHES "^CONFIG_")
		if(DEFINED ${name})
			set(${name} ${${name}} CACHE STRING "")
		endif()
	endif()
endforeach()

set(KCONFIG_H ${APPLICATION_BINARY_DIR}/include/generated/kconfig.h CACHE PATH "")

execute_process(
	COMMAND ${PYTHON_EXECUTABLE}
	${MINIX_RT_BASE}/scripts/mkkconfig_h.py
	-o ${KCONFIG_H}
	OUTPUT_FILE ${KCONFIG_H}
	OUTPUT_QUIET # Discard stdout
	WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
	RESULT_VARIABLE ret
)
if(NOT "${ret}" STREQUAL "0")
	message(FATAL_ERROR "command failed with return code: ${ret}")
endif()
