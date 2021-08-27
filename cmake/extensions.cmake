# SPDX-License-Identifier: Apache-2.0

# https://cmake.org/cmake/help/latest/command/target_sources.html
function(kernel_sources)
	foreach(arg ${ARGV})
		if(IS_DIRECTORY ${arg})
			message(FATAL_ERROR "kernel_sources() was called on a directory")
		endif()
		target_sources(kernel PRIVATE ${arg})
	endforeach()
endfunction()

function(kernel_sources_ifdef feature_toggle)
	if(${${feature_toggle}})
		kernel_sources(${ARGN})
	endif()
endfunction()

function(kernel_sources_ifndef feature_toggle)
	 if(NOT ${feature_toggle})
		kernel_sources(${ARGN})
	endif()
endfunction()

# Constructor with a directory-inferred name
macro(kernel_library)
	kernel_library_get_current_dir_lib_name(${SEL4M_BASE}/kernel lib_name)
	kernel_library_named(${lib_name})
endmacro()

macro(kernel_interface_library)
	kernel_library_get_current_dir_lib_name(${SEL4M_BASE}/kernel lib_name)
	kernel_interface_library_named(${lib_name})
endmacro()

#
# kernel_library versions of normal CMake target_<func> functions
#
function(kernel_library_sources source)
	target_sources(${KERNEL_CURRENT_LIBRARY} PRIVATE ${source} ${ARGN})
endfunction()

function(kernel_library_dependencies target)
	add_dependencies(${KERNEL_CURRENT_LIBRARY} ${target})
endfunction()

function(kernel_interface_library_sources source)
	target_sources(${KERNEL_INTERFACE_CURRENT_LIBRARY} PRIVATE ${source} ${ARGN})
endfunction()

function(kernel_library_sources_ifdef feature_toggle source)
	if(${${feature_toggle}})
		kernel_library_sources(${source} ${ARGN})
	endif()
endfunction()

function(kernel_interface_library_sources_ifdef feature_toggle source)
	if(${${feature_toggle}})
		kernel_interface_library_sources(${source} ${ARGN})
	endif()
endfunction()

function(kernel_library_sources_ifndef feature_toggle source)
	if(NOT ${feature_toggle})
		kernel_library_sources(${source} ${ARGN})
	endif()
endfunction()

function(kernel_interface_library_sources_ifndef feature_toggle source)
	if(NOT ${feature_toggle})
		kernel_interface_library_sources(${source} ${ARGN})
	endif()
endfunction()

function(kernel_library_include_directories)
	target_include_directories(${KERNEL_CURRENT_LIBRARY} PRIVATE ${ARGN})
endfunction()

function(kernel_interface_library_include_directories)
	target_include_directories(${KERNEL_INTERFACE_CURRENT_LIBRARY} PRIVATE ${ARGN})
endfunction()

function(kernel_link_interface_ifdef feature_toggle interface)
	if(${${feature_toggle}})
		target_link_libraries(${interface} INTERFACE kernel_interface)
	endif()
endfunction()

function(services_link_interface_ifdef feature_toggle interface)
	if(${${feature_toggle}})
		target_link_libraries(${interface} INTERFACE services_interface)
	endif()
endfunction()

function(kernel_library_link_libraries item)
	target_link_libraries(${KERNEL_CURRENT_LIBRARY} PUBLIC ${item} ${ARGN})
endfunction()

function(kernel_library_link_libraries_ifdef feature_toggle item)
	if(${${feature_toggle}})
		 kernel_library_link_libraries(${item})
	endif()
endfunction()

function(kernel_interface_library_link_libraries item)
	target_link_libraries(${KERNEL_INTERFACE_CURRENT_LIBRARY} PUBLIC ${item} ${ARGN})
endfunction()

function(kernel_imported_link_libraries item)
	foreach(imported_lib ${ARGN})
		target_link_libraries(${item} PUBLIC ${imported_lib})
	endforeach()
endfunction()

function(kernel_library_compile_definitions item)
	target_compile_definitions(${KERNEL_CURRENT_LIBRARY} PRIVATE ${item} ${ARGN})
endfunction()

function(kernel_library_compile_definitions_ifdef feature_toggle item)
	if(${${feature_toggle}})
		kernel_library_compile_definitions(${item} ${ARGN})
	endif()
endfunction()

function(kernel_library_compile_definitions_ifndef feature_toggle item)
	if(NOT ${${feature_toggle}})
		kernel_library_compile_definitions(${item} ${ARGN})
	endif()
endfunction()

function(kernel_interface_library_compile_definitions item)
	target_compile_definitions(${KERNEL_INTERFACE_CURRENT_LIBRARY} PRIVATE ${item} ${ARGN})
endfunction()

function(kernel_library_compile_options item)
	# The compiler is relied upon for sane behaviour when flags are in
	# conflict. Compilers generally give precedence to flags given late
	# on the command line. So to ensure that kernel_library_* flags are
	# placed late on the command line we create a dummy interface
	# library and link with it to obtain the flags.
	#
	# Linking with a dummy interface library will place flags later on
	# the command line than the the flags from kernel_interface because
	# kernel_interface will be the first interface library that flags
	# are taken from.

	string(MD5 uniqueness ${item})
	set(lib_name options_interface_lib_${uniqueness})

	if (TARGET ${lib_name})
		# ${item} already added, ignoring duplicate just like CMake does
		return()
	endif()

	add_library(           ${lib_name} INTERFACE)
	target_compile_options(${lib_name} INTERFACE ${item} ${ARGN})

	target_link_libraries(${KERNEL_CURRENT_LIBRARY} PRIVATE ${lib_name})
endfunction()

function(kernel_library_compile_options_ifdef feature_toggle item)
	if(${${feature_toggle}})
		kernel_library_compile_options(${item} ${ARGN})
	endif()
endfunction()

function(kernel_interface_library_compile_options item)
	string(MD5 uniqueness ${item})
	set(lib_name options_interface_lib_${uniqueness})

	if (TARGET ${lib_name})
		# ${item} already added, ignoring duplicate just like CMake does
		return()
	endif()

	add_library(           ${lib_name} INTERFACE)
	target_compile_options(${lib_name} INTERFACE ${item} ${ARGN})

	target_link_libraries(${KERNEL_INTERFACE_CURRENT_LIBRARY} PRIVATE ${lib_name})
endfunction()

function(kernel_library_cc_option)
	foreach(option ${ARGV})
		string(MAKE_C_IDENTIFIER check${option} check)
		sel4m_check_compiler_flag(C ${option} ${check})

		if(${check})
			kernel_library_compile_options(${option})
		endif()
	endforeach()
endfunction()

function(kernel_interface_library_cc_option)
	foreach(option ${ARGV})
		string(MAKE_C_IDENTIFIER check${option} check)
		sel4m_check_compiler_flag(C ${option} ${check})

		if(${check})
			kernel_interface_library_compile_options(${option})
		endif()
	endforeach()
endfunction()

# sel4m_check_compiler_flag is a part of sel4m's toolchain
# infrastructure. It should be used when testing toolchain
# capabilities and it should normally be used in place of the
# functions:
#
# check_compiler_flag
# check_c_compiler_flag
# check_cxx_compiler_flag
#
# See check_compiler_flag() for API documentation as it has the same
# API.
#
# It is implemented as a wrapper on top of check_compiler_flag, which
# again wraps the CMake-builtin's check_c_compiler_flag and
# check_cxx_compiler_flag.
#
# It takes time to check for compatibility of flags against toolchains
# so we cache the capability test results in USER_CACHE_DIR (This
# caching comes in addition to the caching that CMake does in the
# build folder's CMakeCache.txt)
function(sel4m_check_compiler_flag lang option check)
	# Check if the option is covered by any hardcoded check before doing
	# an automated test.
	sel4m_check_compiler_flag_hardcoded(${lang} "${option}" check exists)
	if(exists)
		set(check ${check} PARENT_SCOPE)
		return()
	endif()

	# Locate the cache directory
	set_ifndef(
		SEL4M_TOOLCHAIN_CAPABILITY_CACHE_DIR
		${USER_CACHE_DIR}/ToolchainCapabilityDatabase
		)

	# The toolchain capability database/cache is maintained as a
	# directory of files. The filenames in the directory are keys, and
	# the file contents are the values in this key-value store.

	# We need to create a unique key wrt. testing the toolchain
	# capability. This key must include everything that can affect the
	# toolchain test.
	#
	# Also, to fit the key into a filename we calculate the MD5 sum of
	# the key.

	# The 'cacheformat' must be bumped if a bug in the caching mechanism
	# is detected and all old keys must be invalidated.
	set(cacheformat 3)

	set(key_string "")
	set(key_string "${key_string}${cacheformat}_")
	set(key_string "${key_string}${TOOLCHAIN_SIGNATURE}_")
	set(key_string "${key_string}${lang}_")
	set(key_string "${key_string}${option}_")
	set(key_string "${key_string}${CMAKE_REQUIRED_FLAGS}_")

	string(MD5 key ${key_string})

	# Check the cache
	set(key_path ${SEL4M_TOOLCHAIN_CAPABILITY_CACHE_DIR}/${key})
	if(EXISTS ${key_path})
		file(READ
		${key_path}   # File to be read
		key_value     # Output variable
		LIMIT 1       # Read at most 1 byte ('0' or '1')
		)

		set(${check} ${key_value} PARENT_SCOPE)
		return()
	endif()

	# Flags that start with -Wno-<warning> can not be tested by
	# check_compiler_flag, they will always pass, but -W<warning> can be
	# tested, so to test -Wno-<warning> flags we test -W<warning>
	# instead.
	if("${option}" MATCHES "-Wno-(.*)")
		set(possibly_translated_option -W${CMAKE_MATCH_1})
	else()
		set(possibly_translated_option ${option})
	endif()

	check_compiler_flag(${lang} "${possibly_translated_option}" inner_check)

	set(${check} ${inner_check} PARENT_SCOPE)

	# Populate the cache
	if(NOT (EXISTS ${key_path}))

		# This is racy. As often with race conditions, this one can easily be
		# made worse and demonstrated with a simple delay:
		#    execute_process(COMMAND "sleep" "5")
		# Delete the cache, add the sleep above and run twister with a
		# large number of JOBS. Once it's done look at the log.txt file
		# below and you will see that concurrent cmake processes created the
		# same files multiple times.

		# While there are a number of reasons why this race seems both very
		# unlikely and harmless, let's play it safe anyway and write to a
		# private, temporary file first. All modern filesystems seem to
		# support at least one atomic rename API and cmake's file(RENAME
		# ...) officially leverages that.
		string(RANDOM LENGTH 8 tempsuffix)

		file(
			WRITE
			"${key_path}_tmp_${tempsuffix}"
			${inner_check}
			)
		file(
			RENAME
			"${key_path}_tmp_${tempsuffix}" "${key_path}"
			)

		# Populate a metadata file (only intended for trouble shooting)
		# with information about the hash, the toolchain capability
		# result, and the toolchain test.
		file(
			APPEND
			${SEL4M_TOOLCHAIN_CAPABILITY_CACHE_DIR}/log.txt
			"${inner_check} ${key} ${key_string}\n"
			)
	endif()
endfunction()

function(sel4m_check_compiler_flag_hardcoded lang option check exists)
	# Various flags that are not supported for CXX may not be testable
	# because they would produce a warning instead of an error during
	# the test.  Exclude them by toolchain-specific blocklist.
	if((${lang} STREQUAL CXX) AND ("${option}" IN_LIST CXX_EXCLUDED_OPTIONS))
		set(check 0 PARENT_SCOPE)
		set(exists 1 PARENT_SCOPE)
	else()
		# There does not exist a hardcoded check for this option.
		set(exists 0 PARENT_SCOPE)
	endif()
endfunction()

# Determines what the current directory's lib name would be according to the
# provided base and writes it to the argument "lib_name"
macro(kernel_library_get_current_dir_lib_name base lib_name)
	# Remove the prefix (/home/sebo/sel4m/kernel/driver/serial/CMakeLists.txt => driver/serial/CMakeLists.txt)
	file(RELATIVE_PATH name ${base} ${CMAKE_CURRENT_LIST_FILE})

	# Remove the filename (driver/serial/CMakeLists.txt => driver/serial)
	get_filename_component(name ${name} DIRECTORY)

	# Replace / with __ (driver/serial => driver__serial)
	string(REGEX REPLACE "/" "__" name ${name})

	set(${lib_name} ${name})
endmacro()

macro(add_depends_file_touch name)
	add_custom_command(
		TARGET ${name}
		POST_BUILD
		COMMAND
		${SHELL} ${SEL4M_BASE}/scripts/gen_depends_file.sh
		${APPLICATION_BINARY_DIR}
		COMMAND_EXPAND_LISTS
	)
endmacro()

# Constructor with an explicitly given name.
macro(kernel_library_named name)
	# This is a macro because we need add_library() to be executed
	# within the scope of the caller.
	set(KERNEL_CURRENT_LIBRARY ${name})
	add_library(${name} STATIC "")

	add_depends_file_touch(${name})

	kernel_append_cmake_library(${name})

	target_link_libraries(${name} PUBLIC kernel_interface)
endmacro()

macro(kernel_interface_library_named name)
	# This is a macro because we need add_library() to be executed
	# within the scope of the caller.
	set(KERNEL_INTERFACE_CURRENT_LIBRARY ${name})
	add_library(${name} STATIC "")

	add_depends_file_touch(${name})

	kernel_interface_append_cmake_library(${name})

	target_link_libraries(${name} PUBLIC kernel_interface)
endmacro()

# Add the existing CMake library 'library' to the global list of
# kernel CMake libraries. This is done automatically by the
# constructor but must called explicitly on CMake libraries that do
# not use a kernel library constructor.
function(kernel_append_cmake_library library)
	set_property(GLOBAL APPEND PROPERTY KERNEL_BUILT_IN_LIBS ${library})
endfunction()

function(kernel_interface_append_cmake_library library)
	set_property(GLOBAL APPEND PROPERTY KERNEL_INTERFACE_LIBS ${library})
endfunction()

# https://cmake.org/cmake/help/latest/command/target_include_directories.html
function(kernel_include_directories)
	foreach(arg ${ARGV})
		if(IS_ABSOLUTE ${arg})
			set(path ${arg})
		else()
			set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
		endif()
		target_include_directories(kernel_interface INTERFACE ${path})
	endforeach()
endfunction()

function(services_include_directories)
	foreach(arg ${ARGV})
		if(IS_ABSOLUTE ${arg})
			set(path ${arg})
		else()
			set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
		endif()
		target_include_directories(services_interface INTERFACE ${path})
	endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_include_directories.html
function(kernel_system_include_directories)
	foreach(arg ${ARGV})
		if(IS_ABSOLUTE ${arg})
			set(path ${arg})
		else()
			set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
		endif()
		target_include_directories(kernel_interface SYSTEM INTERFACE ${path})
	endforeach()
endfunction()

function(services_system_include_directories)
	foreach(arg ${ARGV})
		if(IS_ABSOLUTE ${arg})
			set(path ${arg})
		else()
			set(path ${CMAKE_CURRENT_SOURCE_DIR}/${arg})
		endif()
		target_include_directories(services_interface SYSTEM INTERFACE ${path})
	endforeach()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_compile_definitions.html
function(kernel_compile_definitions)
	target_compile_definitions(kernel_interface INTERFACE ${ARGV})
endfunction()

function(services_compile_definitions)
	target_compile_definitions(services_interface INTERFACE ${ARGV})
endfunction()

function(kernel_compile_definitions_ifdef feature_toggle)
	if(${${feature_toggle}})
		kernel_compile_definitions(${ARGN})
	endif()
endfunction()

function(services_compile_definitions_ifdef feature_toggle)
	if(${${feature_toggle}})
		services_compile_definitions(${ARGN})
	endif()
endfunction()

function(kernel_compile_definitions_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		kernel_compile_definitions(${ARGN})
	endif()
endfunction()

function(services_compile_definitions_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		services_compile_definitions(${ARGN})
	endif()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_compile_options.html
function(kernel_compile_options)
	target_compile_options(kernel_interface INTERFACE ${ARGV})
endfunction()

function(services_compile_options)
	target_compile_options(services_interface INTERFACE ${ARGV})
endfunction()

function(kernel_compile_options_ifdef feature_toggle)
	if(${${feature_toggle}})
		kernel_compile_options(${ARGN})
	endif()
endfunction()

function(services_compile_options_ifdef feature_toggle)
	if(${${feature_toggle}})
		services_compile_options(${ARGN})
	endif()
endfunction()

function(kernel_compile_options_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		kernel_compile_options(${ARGN})
	endif()
endfunction()

function(services_compile_options_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		services_compile_options(${ARGN})
	endif()
endfunction()

# https://cmake.org/cmake/help/latest/command/target_link_libraries.html
function(kernel_link_libraries)
	target_link_libraries(kernel_interface INTERFACE ${ARGV})
endfunction()

function(services_link_libraries)
	target_link_libraries(services_interface INTERFACE ${ARGV})
endfunction()

function(kernel_link_libraries_ifdef feature_toggle)
	if(${${feature_toggle}})
		kernel_link_libraries(${ARGN})
	endif()
endfunction()

function(services_link_libraries_ifdef feature_toggle)
	if(${${feature_toggle}})
		services_link_libraries(${ARGN})
	endif()
endfunction()

function(kernel_link_libraries_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		kernel_link_libraries(${ARGN})
	endif()
endfunction()

function(services_link_libraries_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		services_link_libraries(${ARGN})
	endif()
endfunction()

function(kernel_cc_option)
	foreach(arg ${ARGV})
		target_cc_option(kernel_interface INTERFACE ${arg})
	endforeach()
endfunction()

function(services_cc_option)
	foreach(arg ${ARGV})
		target_cc_option(services_interface INTERFACE ${arg})
	endforeach()
endfunction()

function(kernel_cc_option_ifdef feature_toggle)
	if(${${feature_toggle}})
		kernel_cc_option(${ARGN})
	endif()
endfunction()

function(services_cc_option_ifdef feature_toggle)
	if(${${feature_toggle}})
		services_cc_option(${ARGN})
	endif()
endfunction()

function(kernel_cc_option_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		kernel_cc_option(${ARGN})
	endif()
endfunction()

function(services_cc_option_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		services_cc_option(${ARGN})
	endif()
endfunction()

function(kernel_cc_option_fallback option1 option2)
		target_cc_option_fallback(kernel_interface INTERFACE ${option1} ${option2})
endfunction()

function(services_cc_option_fallback option1 option2)
		target_cc_option_fallback(services_interface INTERFACE ${option1} ${option2})
endfunction()

function(kernel_ld_options)
		target_ld_options(kernel_interface INTERFACE ${ARGV})
endfunction()

function(services_ld_options)
		target_ld_options(services_interface INTERFACE ${ARGV})
endfunction()

function(kernel_ld_option_ifdef feature_toggle)
	if(${${feature_toggle}})
		kernel_ld_options(${ARGN})
	endif()
endfunction()

function(services_ld_option_ifdef feature_toggle)
	if(${${feature_toggle}})
		services_ld_options(${ARGN})
	endif()
endfunction()

function(kernel_ld_option_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		kernel_ld_options(${ARGN})
	endif()
endfunction()

function(services_ld_option_ifndef feature_toggle)
	if(NOT ${${feature_toggle}})
		services_ld_options(${ARGN})
	endif()
endfunction()




# Getter functions for extracting build information from
# kernel_interface. Returning lists, and strings is supported, as is
# requesting specific categories of build information (defines,
# includes, options).
#
# The naming convention follows:
# kernel_get_${build_information}_for_lang${format}(lang x [STRIP_PREFIX])
# Where
#  the argument 'x' is written with the result
# and
#  ${build_information} can be one of
#   - include_directories           # -I directories
#   - system_include_directories    # -isystem directories
#   - compile_definitions           # -D'efines
#   - compile_options               # misc. compiler flags
# and
#  ${format} can be
#   - the empty string '', signifying that it should be returned as a list
#   - _as_string signifying that it should be returned as a string
# and
#  ${lang} can be one of
#   - C
#   - CXX
#   - ASM
#
# STRIP_PREFIX
#
# By default the result will be returned ready to be passed directly
# to a compiler, e.g. prefixed with -D, or -I, but it is possible to
# omit this prefix by specifying 'STRIP_PREFIX' . This option has no
# effect for 'compile_options'.
#
# e.g.
# kernel_get_include_directories_for_lang(ASM x)
# writes "-Isome_dir;-Isome/other/dir" to x

function(kernel_get_include_directories_for_lang_as_string lang i)
	kernel_get_include_directories_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(services_get_include_directories_for_lang_as_string lang i)
	services_get_include_directories_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(kernel_get_system_include_directories_for_lang_as_string lang i)
	kernel_get_system_include_directories_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(services_get_system_include_directories_for_lang_as_string lang i)
	services_get_system_include_directories_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(kernel_get_compile_definitions_for_lang_as_string lang i)
	kernel_get_compile_definitions_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(services_get_compile_definitions_for_lang_as_string lang i)
	services_get_compile_definitions_for_lang(${lang} list_of_flags DELIMITER " " ${ARGN})

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(kernel_get_compile_options_for_lang_as_string lang i)
	kernel_get_compile_options_for_lang(${lang} list_of_flags DELIMITER " ")

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(services_get_compile_options_for_lang_as_string lang i)
	services_get_compile_options_for_lang(${lang} list_of_flags DELIMITER " ")

	convert_list_of_flags_to_string_of_flags(list_of_flags str_of_flags)

	set(${i} ${str_of_flags} PARENT_SCOPE)
endfunction()

function(kernel_get_include_directories_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET kernel_interface PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	if(NOT ARGN)
		set(result_output_list "-I$<JOIN:${genexp_output_list},$<SEMICOLON>-I>")
	elseif(args_STRIP_PREFIX)
		# The list has no prefix, so don't add it.
		set(result_output_list ${output_list})
	elseif(args_DELIMITER)
		set(result_output_list "-I$<JOIN:${genexp_output_list},${args_DELIMITER}-I>")
	endif()
	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(services_get_include_directories_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET services_interface PROPERTY INTERFACE_INCLUDE_DIRECTORIES)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	if(NOT ARGN)
		set(result_output_list "-I$<JOIN:${genexp_output_list},$<SEMICOLON>-I>")
	elseif(args_STRIP_PREFIX)
		# The list has no prefix, so don't add it.
		set(result_output_list ${output_list})
	elseif(args_DELIMITER)
		set(result_output_list "-I$<JOIN:${genexp_output_list},${args_DELIMITER}-I>")
	endif()
	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(kernel_get_system_include_directories_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET kernel_interface PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	set_ifndef(args_DELIMITER "$<SEMICOLON>")
	set(result_output_list "$<$<BOOL:${genexp_output_list}>:-isystem$<JOIN:${genexp_output_list},${args_DELIMITER}-isystem>>")

	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(services_get_system_include_directories_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET services_interface PROPERTY INTERFACE_SYSTEM_INCLUDE_DIRECTORIES)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	set_ifndef(args_DELIMITER "$<SEMICOLON>")
	set(result_output_list "$<$<BOOL:${genexp_output_list}>:-isystem$<JOIN:${genexp_output_list},${args_DELIMITER}-isystem>>")

	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(kernel_get_compile_definitions_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET kernel_interface PROPERTY INTERFACE_COMPILE_DEFINITIONS)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	set_ifndef(args_DELIMITER "$<SEMICOLON>")
	set(result_output_list "-D$<JOIN:${genexp_output_list},${args_DELIMITER}-D>")

	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(services_get_compile_definitions_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET services_interface PROPERTY INTERFACE_COMPILE_DEFINITIONS)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	set_ifndef(args_DELIMITER "$<SEMICOLON>")
	set(result_output_list "-D$<JOIN:${genexp_output_list},${args_DELIMITER}-D>")

	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(kernel_get_compile_options_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET kernel_interface PROPERTY INTERFACE_COMPILE_OPTIONS)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	set_ifndef(args_DELIMITER "$<SEMICOLON>")
	set(result_output_list "$<JOIN:${genexp_output_list},${args_DELIMITER}>")

	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

function(services_get_compile_options_for_lang lang i)
	sel4m_get_parse_args(args ${ARGN})
	get_property(flags TARGET services_interface PROPERTY INTERFACE_COMPILE_OPTIONS)

	process_flags(${lang} flags output_list)
	string(REPLACE ";" "$<SEMICOLON>" genexp_output_list "${output_list}")

	set_ifndef(args_DELIMITER "$<SEMICOLON>")
	set(result_output_list "$<JOIN:${genexp_output_list},${args_DELIMITER}>")

	set(${i} ${result_output_list} PARENT_SCOPE)
endfunction()

# This function writes a dict to it's output parameter
# 'return_dict'. The dict has information about the parsed arguments,
#
# Usage:
#   sel4m_get_parse_args(foo ${ARGN})
#   print(foo_STRIP_PREFIX) # foo_STRIP_PREFIX might be set to 1
function(sel4m_get_parse_args return_dict)
	foreach(x ${ARGN})
		if(DEFINED single_argument)
			set(${single_argument} ${x} PARENT_SCOPE)
			unset(single_argument)
		else()
			if(x STREQUAL STRIP_PREFIX)
				set(${return_dict}_STRIP_PREFIX 1 PARENT_SCOPE)
			elseif(x STREQUAL NO_SPLIT)
				set(${return_dict}_NO_SPLIT 1 PARENT_SCOPE)
			elseif(x STREQUAL DELIMITER)
				set(single_argument ${return_dict}_DELIMITER)
			endif()
		endif()
	endforeach()
endfunction()

function(process_flags lang input output)
	# The flags might contains compile language generator expressions that
	# look like this:
	# $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
	# $<$<COMPILE_LANGUAGE:CXX>:$<OTHER_EXPRESSION>>
	#
	# Flags that don't specify a language like this apply to all
	# languages.
	#
	# See COMPILE_LANGUAGE in
	# https://cmake.org/cmake/help/v3.3/manual/cmake-generator-expressions.7.html
	#
	# To deal with this, we apply a regex to extract the flag and also
	# to find out if the language matches.
	#
	# If this doesn't work out we might need to ban the use of
	# COMPILE_LANGUAGE and instead partition C, CXX, and ASM into
	# different libraries
	set(languages C CXX ASM)

	set(tmp_list "")

	foreach(flag ${${input}})
		set(is_compile_lang_generator_expression 0)
		foreach(l ${languages})
			if(flag MATCHES "<COMPILE_LANGUAGE:${l}>:([^>]+)>")
				set(updated_flag ${CMAKE_MATCH_1})
				set(is_compile_lang_generator_expression 1)
				if(${l} STREQUAL ${lang})
					# This test will match in case there are more generator expressions in the flag.
					# As example: $<$<COMPILE_LANGUAGE:C>:$<OTHER_EXPRESSION>>
					#             $<$<OTHER_EXPRESSION:$<COMPILE_LANGUAGE:C>:something>>
					string(REGEX MATCH "(\\\$<)[^\\\$]*(\\\$<)[^\\\$]*(\\\$<)" IGNORE_RESULT ${flag})
					if(CMAKE_MATCH_2)
						# Nested generator expressions are used, just substitue `$<COMPILE_LANGUAGE:${l}>` to `1`
						string(REGEX REPLACE "\\\$<COMPILE_LANGUAGE:${l}>" "1" updated_flag ${flag})
					endif()
					list(APPEND tmp_list ${updated_flag})
					break()
				endif()
			endif()
		endforeach()

		if(NOT is_compile_lang_generator_expression)
			# SHELL is used to avoid de-deplucation, but when process flags
			# then this tag must be removed to return real compile/linker flags.
			if(flag MATCHES "SHELL:[ ]*(.*)")
				separate_arguments(flag UNIX_COMMAND ${CMAKE_MATCH_1})
			endif()
			# Flags may be placed inside generator expression, therefore any flag
			# which is not already a generator expression must have commas converted.
			if(NOT flag MATCHES "\\\$<.*>")
				string(REPLACE "," "$<COMMA>" flag "${flag}")
			endif()
			list(APPEND tmp_list ${flag})
		endif()
	endforeach()

	set(${output} ${tmp_list} PARENT_SCOPE)
endfunction()

function(convert_list_of_flags_to_string_of_flags ptr_list_of_flags string_of_flags)
	# Convert the list to a string so we can do string replace
	# operations on it and replace the ";" list separators with a
	# whitespace so the flags are spaced out
	string(REPLACE ";"  " "  locally_scoped_string_of_flags "${${ptr_list_of_flags}}")

	# Set the output variable in the parent scope
	set(${string_of_flags} ${locally_scoped_string_of_flags} PARENT_SCOPE)
endfunction()

macro(get_property_and_add_prefix result target property prefix)
	sel4m_get_parse_args(args ${ARGN})

	if(args_STRIP_PREFIX)
		set(maybe_prefix "")
	else()
		set(maybe_prefix ${prefix})
	endif()

	get_property(target_property TARGET ${target} PROPERTY ${property})
	foreach(x ${target_property})
		list(APPEND ${result} ${maybe_prefix}${x})
	endforeach()
endmacro()

# These functions are useful if there is a need to generate a file
# that can be included into the application at build time. The file
# can also be compressed automatically when embedding it.
#
# See tests/application_development/gen_inc_file for an example of
# usage.
function(generate_inc_file
		source_file    # The source file to be converted to hex
		generated_file # The generated file
		)
	add_custom_command(
		OUTPUT ${generated_file}
		COMMAND
		${PYTHON_EXECUTABLE}
		${SEL4M_BASE}/scripts/file2hex.py
		${ARGN} # Extra arguments are passed to file2hex.py
		--file ${source_file}
		> ${generated_file} # Does pipe redirection work on Windows?
		DEPENDS ${source_file}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		)
endfunction()

function(generate_inc_file_for_gen_target
		target          # The cmake target that depends on the generated file
		source_file     # The source file to be converted to hex
		generated_file  # The generated file
		gen_target      # The generated file target we depend on
										# Any additional arguments are passed on to file2hex.py
		)
	generate_inc_file(${source_file} ${generated_file} ${ARGN})

	# Ensure 'generated_file' is generated before 'target' by creating a
	# dependency between the two targets

	add_dependencies(${target} ${gen_target})
endfunction()

function(generate_inc_file_for_target
		target          # The cmake target that depends on the generated file
		source_file     # The source file to be converted to hex
		generated_file  # The generated file
										# Any additional arguments are passed on to file2hex.py
		)
	# Ensure 'generated_file' is generated before 'target' by creating a
	# 'custom_target' for it and setting up a dependency between the two
	# targets

	# But first create a unique name for the custom target
	generate_unique_target_name_from_filename(${generated_file} generated_target_name)

	add_custom_target(${generated_target_name} DEPENDS ${generated_file})
	generate_inc_file_for_gen_target(${target} ${source_file} ${generated_file} ${generated_target_name} ${ARGN})
endfunction()

# Usage:
#   check_dtc_flag("-Wtest" DTC_WARN_TEST)
#
# Writes 1 to the output variable 'ok' if
# the flag is supported, otherwise writes 0.
#
# using
function(check_dtc_flag flag ok)
	execute_process(
		COMMAND
		${DTC} ${flag} -v
		ERROR_QUIET
		OUTPUT_QUIET
		RESULT_VARIABLE dtc_check_ret
	)
	if (dtc_check_ret EQUAL 0)
		set(${ok} 1 PARENT_SCOPE)
	else()
		set(${ok} 0 PARENT_SCOPE)
	endif()
endfunction()

#
# import_kconfig(<prefix> <kconfig_fragment> [<keys>])
#
# Parse a KConfig fragment (typically with extension .config) and
# introduce all the symbols that are prefixed with 'prefix' into the
# CMake namespace. List all created variable names in the 'keys'
# output variable if present.
function(import_kconfig prefix kconfig_fragment)
	# Parse the lines prefixed with 'prefix' in ${kconfig_fragment}
	file(
		STRINGS
		${kconfig_fragment}
		DOT_CONFIG_LIST
		REGEX "^${prefix}"
		ENCODING "UTF-8"
	)

	foreach (CONFIG ${DOT_CONFIG_LIST})
		# CONFIG could look like: CONFIG_NET_BUF=y

		# Match the first part, the variable name
		string(REGEX MATCH "[^=]+" CONF_VARIABLE_NAME ${CONFIG})

		# Match the second part, variable value
		string(REGEX MATCH "=(.+$)" CONF_VARIABLE_VALUE ${CONFIG})
		# The variable name match we just did included the '=' symbol. To just get the
		# part on the RHS we use match group 1
		set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})

		if("${CONF_VARIABLE_VALUE}" MATCHES "^\"(.*)\"$") # Is surrounded by quotes
			set(CONF_VARIABLE_VALUE ${CMAKE_MATCH_1})
		endif()

		set("${CONF_VARIABLE_NAME}" "${CONF_VARIABLE_VALUE}" PARENT_SCOPE)
		list(APPEND keys "${CONF_VARIABLE_NAME}")
	endforeach()

	foreach(outvar ${ARGN})
		set(${outvar} "${keys}" PARENT_SCOPE)
	endforeach()
endfunction()

#
# ifdef functions are added on an as-need basis. See
# https://cmake.org/cmake/help/latest/manual/cmake-commands.7.html for
# a list of available functions.
function(add_subdirectory_ifdef feature_toggle source_dir)
	if(${${feature_toggle}})
		add_subdirectory(${source_dir} ${ARGN})
	endif()
endfunction()

function(target_sources_ifdef feature_toggle target scope item)
	if(${${feature_toggle}})
		target_sources(${target} ${scope} ${item} ${ARGN})
	endif()
endfunction()

function(target_compile_definitions_ifdef feature_toggle target scope item)
	if(${${feature_toggle}})
		target_compile_definitions(${target} ${scope} ${item} ${ARGN})
	endif()
endfunction()

function(target_include_directories_ifdef feature_toggle target scope item)
	if(${${feature_toggle}})
		target_include_directories(${target} ${scope} ${item} ${ARGN})
	endif()
endfunction()

function(target_link_libraries_ifdef feature_toggle target item)
	if(${${feature_toggle}})
		target_link_libraries(${target} ${item} ${ARGN})
	endif()
endfunction()

function(add_compile_option_ifdef feature_toggle option)
	if(${${feature_toggle}})
		add_compile_options(${option})
	endif()
endfunction()

function(target_compile_option_ifdef feature_toggle target scope option)
	if(${feature_toggle})
		target_compile_options(${target} ${scope} ${option})
	endif()
endfunction()

function(target_cc_option_ifdef feature_toggle target scope option)
	if(${feature_toggle})
		target_cc_option(${target} ${scope} ${option})
	endif()
endfunction()

macro(list_append_ifdef feature_toggle list)
	if(${${feature_toggle}})
		list(APPEND ${list} ${ARGN})
	endif()
endmacro()

function(set_ifndef variable value)
	if(NOT ${variable})
		set(${variable} ${value} ${ARGN} PARENT_SCOPE)
	endif()
endfunction()

function(target_cc_option_ifndef feature_toggle target scope option)
	if(NOT ${feature_toggle})
		target_cc_option(${target} ${scope} ${option})
	endif()
endfunction()

#
# Utility functions for silently omitting compiler flags when the
# compiler lacks support. *_cc_option was ported from KBuild, see
# cc-option in
# https://www.kernel.org/doc/Documentation/kbuild/makefiles.txt

# Writes 1 to the output variable 'ok' for the language 'lang' if
# the flag is supported, otherwise writes 0.
#
# lang must be C or CXX
#
# TODO: Support ASM
#
# Usage:
#
# check_compiler_flag(C "-Wall" my_check)
# print(my_check) # my_check is now 1
function(check_compiler_flag lang option ok)
	if(NOT DEFINED CMAKE_REQUIRED_QUIET)
		set(CMAKE_REQUIRED_QUIET 1)
	endif()

	string(MAKE_C_IDENTIFIER
		check${option}_${lang}_${CMAKE_REQUIRED_FLAGS}
		${ok}
		)

	if(${lang} STREQUAL C)
		check_c_compiler_flag("${option}" ${${ok}})
	else()
		check_cxx_compiler_flag("${option}" ${${ok}})
	endif()

	if(${${${ok}}})
		set(ret 1)
	else()
		set(ret 0)
	endif()

	set(${ok} ${ret} PARENT_SCOPE)
endfunction()

function(target_cc_option target scope option)
	target_cc_option_fallback(${target} ${scope} ${option} "")
endfunction()

# Support an optional second option for when the first option is not
# supported.
function(target_cc_option_fallback target scope option1 option2)
	if(CONFIG_CPLUSPLUS)
		foreach(lang C CXX)
			# For now, we assume that all flags that apply to C/CXX also
			# apply to ASM.
			sel4m_check_compiler_flag(${lang} ${option1} check)
			if(${check})
				target_compile_options(${target} ${scope}
					$<$<COMPILE_LANGUAGE:${lang}>:${option1}>
					$<$<COMPILE_LANGUAGE:ASM>:${option1}>
					)
			elseif(option2)
				target_compile_options(${target} ${scope}
					$<$<COMPILE_LANGUAGE:${lang}>:${option2}>
					$<$<COMPILE_LANGUAGE:ASM>:${option2}>
					)
			endif()
		endforeach()
	else()
		sel4m_check_compiler_flag(C ${option1} check)
		if(${check})
			target_compile_options(${target} ${scope} ${option1})
		elseif(option2)
			target_compile_options(${target} ${scope} ${option2})
		endif()
	endif()
endfunction()

function(target_ld_options target scope)
	sel4m_get_parse_args(args ${ARGN})
	list(REMOVE_ITEM ARGN NO_SPLIT)

	foreach(option ${ARGN})
		if(args_NO_SPLIT)
			set(option ${ARGN})
		endif()
		string(JOIN "" check_identifier "check" ${option})
		string(MAKE_C_IDENTIFIER ${check_identifier} check)

		set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
		string(JOIN " " CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS} ${option})
		sel4m_check_compiler_flag(C "" ${check})
		set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

		target_link_libraries_ifdef(${check} ${target} ${scope} ${option})

		if(args_NO_SPLIT)
			break()
		endif()
	endforeach()
endfunction()

#
# 'toolchain_parse_make_rule' is a function that parses the output of
# 'gcc -M'.
#
# The argument 'input_file' is in input parameter with the path to the
# file with the dependency information.
#
# The argument 'include_files' is an output parameter with the result
# of parsing the include files.
function(toolchain_parse_make_rule input_file include_files)
	file(READ ${input_file} input)

	# The file is formatted like this:
	# empty_file.o: misc/empty_file.c \
	# nrf52840dk_nrf52840/nrf52840dk_nrf52840.dts \
	# nrf52840_qiaa.dtsi

	# Get rid of the backslashes
	string(REPLACE "\\" ";" input_as_list ${input})

	# Pop the first line and treat it specially
	list(GET input_as_list 0 first_input_line)
	string(FIND ${first_input_line} ": " index)
	math(EXPR j "${index} + 2")
	string(SUBSTRING ${first_input_line} ${j} -1 first_include_file)
	list(REMOVE_AT input_as_list 0)

	list(APPEND result ${first_include_file})

	# Add the other lines
	list(APPEND result ${input_as_list})

	# Strip away the newlines and whitespaces
	list(TRANSFORM result STRIP)

	set(${include_files} ${result} PARENT_SCOPE)
endfunction()

# 'check_set_linker_property' is a function that check the provided linker
# flag and only set the linker property if the check succeeds
#
# This function is similar in nature to the CMake set_property function, but
# with the extension that it will check that the linker supports the flag before
# setting the property.
#
# APPEND: Flag indicated that the property should be appended to the existing
#         value list for the property.
# TARGET: Name of target on which to add the property (commonly: linker)
# PROPERTY: Name of property with the value(s) following immediately after
#           property name
function(check_set_linker_property)
	set(options APPEND)
	set(single_args TARGET)
	set(multi_args  PROPERTY)
	cmake_parse_arguments(LINKER_PROPERTY "${options}" "${single_args}" "${multi_args}" ${ARGN})

	if(LINKER_PROPERTY_APPEND)
	 set(APPEND "APPEND")
	endif()

	list(GET LINKER_PROPERTY_PROPERTY 0 property)
	list(REMOVE_AT LINKER_PROPERTY_PROPERTY 0)
	set(option ${LINKER_PROPERTY_PROPERTY})

	string(MAKE_C_IDENTIFIER check${option} check)

	set(SAVED_CMAKE_REQUIRED_FLAGS ${CMAKE_REQUIRED_FLAGS})
	set(CMAKE_REQUIRED_FLAGS "${CMAKE_REQUIRED_FLAGS} ${option}")
	sel4m_check_compiler_flag(C "" ${check})
	set(CMAKE_REQUIRED_FLAGS ${SAVED_CMAKE_REQUIRED_FLAGS})

	if(${check})
		set_property(TARGET ${LINKER_PROPERTY_TARGET} ${APPEND} PROPERTY ${property} ${option})
	endif()
endfunction()

# 'set_compiler_property' is a function that sets the property for the C and
# C++ property targets used for toolchain abstraction.
#
# This function is similar in nature to the CMake set_property function, but
# with the extension that it will set the property on both the compile and
# compiler-cpp targets.
#
# APPEND: Flag indicated that the property should be appended to the existing
#         value list for the property.
# PROPERTY: Name of property with the value(s) following immediately after
#           property name
function(set_compiler_property)
	set(options APPEND)
	set(multi_args  PROPERTY)
	cmake_parse_arguments(COMPILER_PROPERTY "${options}" "${single_args}" "${multi_args}" ${ARGN})
	if(COMPILER_PROPERTY_APPEND)
	 set(APPEND "APPEND")
	 set(APPEND-CPP "APPEND")
	endif()

	set_property(TARGET compiler ${APPEND} PROPERTY ${COMPILER_PROPERTY_PROPERTY})
	set_property(TARGET compiler-cpp ${APPEND} PROPERTY ${COMPILER_PROPERTY_PROPERTY})
endfunction()

# 'check_set_compiler_property' is a function that check the provided compiler
# flag and only set the compiler or compiler-cpp property if the check succeeds
#
# This function is similar in nature to the CMake set_property function, but
# with the extension that it will check that the compiler supports the flag
# before setting the property on compiler or compiler-cpp targets.
#
# APPEND: Flag indicated that the property should be appended to the existing
#         value list for the property.
# PROPERTY: Name of property with the value(s) following immediately after
#           property name
function(check_set_compiler_property)
	set(options APPEND)
	set(multi_args  PROPERTY)
	cmake_parse_arguments(COMPILER_PROPERTY "${options}" "${single_args}" "${multi_args}" ${ARGN})
	if(COMPILER_PROPERTY_APPEND)
	 set(APPEND "APPEND")
	 set(APPEND-CPP "APPEND")
	endif()

	list(GET COMPILER_PROPERTY_PROPERTY 0 property)
	list(REMOVE_AT COMPILER_PROPERTY_PROPERTY 0)

	foreach(option ${COMPILER_PROPERTY_PROPERTY})
		if(CONFIG_CPLUSPLUS)
			sel4m_check_compiler_flag(CXX ${option} check)

			if(${check})
				set_property(TARGET compiler-cpp ${APPEND-CPP} PROPERTY ${property} ${option})
				set(APPEND-CPP "APPEND")
			endif()
		endif()

		sel4m_check_compiler_flag(C ${option} check)

		if(${check})
			set_property(TARGET compiler ${APPEND} PROPERTY ${property} ${option})
			set(APPEND "APPEND")
		endif()
	endforeach()
endfunction()

# Usage:
#   print(BOARD)
#
# will print: "BOARD: nrf52dk_nrf52832"
function(print arg)
	message(STATUS "${arg}: ${${arg}}")
endfunction()

# Usage:
#   assert(SEL4M_TOOLCHAIN "SEL4M_TOOLCHAIN not set.")
#
# will cause a FATAL_ERROR and print an error message if the first
# expression is false
macro(assert test comment)
	if(NOT ${test})
		message(FATAL_ERROR "Assertion failed: ${comment}")
	endif()
endmacro()

# Usage:
#   assert_not(OBSOLETE_VAR "OBSOLETE_VAR has been removed; use NEW_VAR instead")
#
# will cause a FATAL_ERROR and print an error message if the first
# expression is true
macro(assert_not test comment)
	if(${test})
		message(FATAL_ERROR "Assertion failed: ${comment}")
	endif()
endmacro()

# Usage:
#   assert_exists(CMAKE_READELF)
#
# will cause a FATAL_ERROR if there is no file or directory behind the
# variable
macro(assert_exists var)
	if(NOT EXISTS ${${var}})
		message(FATAL_ERROR "No such file or directory: ${var}: '${${var}}'")
	endif()
endmacro()

function(check_if_directory_is_writeable dir ok)
	execute_process(
		COMMAND
		${PYTHON_EXECUTABLE}
		${SEL4M_BASE}/scripts/dir_is_writeable.py
		${dir}
		RESULT_VARIABLE ret
		)

	if("${ret}" STREQUAL "0")
		# The directory is write-able
		set(${ok} 1 PARENT_SCOPE)
	else()
		set(${ok} 0 PARENT_SCOPE)
	endif()
endfunction()

function(find_appropriate_cache_directory dir)
	set(env_suffix_LOCALAPPDATA   .cache)

	if(CMAKE_HOST_APPLE)
		# On macOS, ~/Library/Caches is the preferred cache directory.
		set(env_suffix_HOME Library/Caches)
	else()
		set(env_suffix_HOME .cache)
	endif()

	# Determine which env vars should be checked
	if(CMAKE_HOST_APPLE)
		set(dirs HOME)
	elseif(CMAKE_HOST_WIN32)
		set(dirs LOCALAPPDATA)
	else()
		# Assume Linux when we did not detect 'mac' or 'win'
		#
		# On Linux, freedesktop.org recommends using $XDG_CACHE_HOME if
		# that is defined and defaulting to $HOME/.cache otherwise.
		set(dirs
			XDG_CACHE_HOME
			HOME
			)
	endif()

	foreach(env_var ${dirs})
		if(DEFINED ENV{${env_var}})
			set(env_dir $ENV{${env_var}})

			set(test_user_dir ${env_dir}/${env_suffix_${env_var}})

			check_if_directory_is_writeable(${test_user_dir} ok)
			if(${ok})
				# The directory is write-able
				set(user_dir ${test_user_dir})
				break()
			else()
				# The directory was not writeable, keep looking for a suitable
				# directory
			endif()
		endif()
	endforeach()

	# Populate local_dir with a suitable directory for caching
	# files. Prefer a directory outside of the git repository because it
	# is good practice to have clean git repositories.
	if(DEFINED user_dir)
		# sel4m's cache files go in the "sel4m" subdirectory of the
		# user's cache directory.
		set(local_dir ${user_dir}/sel4m)
	else()
		set(local_dir ${SEL4M_BASE}/.cache)
	endif()

	set(${dir} ${local_dir} PARENT_SCOPE)
endfunction()

function(generate_unique_target_name_from_filename filename target_name)
	get_filename_component(basename ${filename} NAME)
	string(REPLACE "." "_" x ${basename})
	string(REPLACE "@" "_" x ${x})

	string(MD5 unique_chars ${filename})

	set(${target_name} gen_${x}_${unique_chars} PARENT_SCOPE)
endfunction()

# Usage:
#   sel4m_file(<mode> <arg> ...)
#
# sel4m file function extension.
# This function currently support the following <modes>
#
# APPLICATION_ROOT <path>: Check all paths in provided variable, and convert
#                          those paths that are defined with `-D<path>=<val>`
#                          to absolute path, relative from `APPLICATION_SOURCE_DIR`
#                          Issue an error for any relative path not specified
#                          by user with `-D<path>`
#
# CONF_FILES <path>: Nothing to do
#
# returns an updated list of absolute paths
function(sel4m_file)
	set(file_options APPLICATION_ROOT CONF_FILES)
	if((ARGC EQUAL 0) OR (NOT (ARGV0 IN_LIST file_options)))
		message(FATAL_ERROR "No <mode> given to `sel4m_file(<mode> <args>...)` function,\n \
Please provide one of following: APPLICATION_ROOT, CONF_FILES")
	endif()

	if(${ARGV0} STREQUAL APPLICATION_ROOT)
		set(single_args APPLICATION_ROOT)
	elseif(${ARGV0} STREQUAL CONF_FILES)
		set(single_args CONF_FILES BOARD BOARD_REVISION DTS KCONF BUILD)
	endif()

	cmake_parse_arguments(FILE "" "${single_args}" "" ${ARGN})
	if(FILE_UNPARSED_ARGUMENTS)
			message(FATAL_ERROR "sel4m_file(${ARGV0} <path> ...) given unknown arguments: ${FILE_UNPARSED_ARGUMENTS}")
	endif()


	if(FILE_APPLICATION_ROOT)
		# Note: user can do: `-D<var>=<relative-path>` and app can at same
		# time specify `list(APPEND <var> <abs-path>)`
		# Thus need to check and update only CACHED variables (-D<var>).
		set(CACHED_PATH $CACHE{${FILE_APPLICATION_ROOT}})
		foreach(path ${CACHED_PATH})
			# The cached variable is relative path, i.e. provided by `-D<var>` or
			# `set(<var> CACHE)`, so let's update current scope variable to absolute
			# path from  `APPLICATION_SOURCE_DIR`.
			if(NOT IS_ABSOLUTE ${path})
				set(abs_path ${APPLICATION_SOURCE_DIR}/${path})
				list(FIND ${FILE_APPLICATION_ROOT} ${path} index)
				if(NOT ${index} LESS 0)
					list(REMOVE_AT ${FILE_APPLICATION_ROOT} ${index})
					list(INSERT ${FILE_APPLICATION_ROOT} ${index} ${abs_path})
				endif()
			endif()
		endforeach()

		# Now all cached relative paths has been updated.
		# Let's check if anyone uses relative path as scoped variable, and fail
		foreach(path ${${FILE_APPLICATION_ROOT}})
			if(NOT IS_ABSOLUTE ${path})
				message(FATAL_ERROR
"Relative path encountered in scoped variable: ${FILE_APPLICATION_ROOT}, value=${path}\n \
Please adjust any `set(${FILE_APPLICATION_ROOT} ${path})` or `list(APPEND ${FILE_APPLICATION_ROOT} ${path})`\n \
to absolute path using `\${CMAKE_CURRENT_SOURCE_DIR}/${path}` or similar. \n \
Relative paths are only allowed with `-D${ARGV1}=<path>`")
			endif()
		endforeach()

		# This updates the provided argument in parent scope (callers scope)
		set(${FILE_APPLICATION_ROOT} ${${FILE_APPLICATION_ROOT}} PARENT_SCOPE)
	endif()

	if(FILE_CONF_FILES)
		message(WARNING "Do not implement.")
	endif()
endfunction()

# Usage:
#   sel4m_string(<mode> <out-var> <input> ...)
#
# sel4m string function extension.
# This function extends the CMake string function by providing additional
# manipulation arguments to CMake string.
#
# SANITIZE: Ensure that the output string does not contain any special
#           characters. Special characters, such as -, +, =, $, etc. are
#           converted to underscores '_'.
#
# SANITIZE TOUPPER: Ensure that the output string does not contain any special
#                   characters. Special characters, such as -, +, =, $, etc. are
#                   converted to underscores '_'.
#                   The sanitized string will be returned in UPPER case.
#
# returns the updated string
function(sel4m_string)
	set(options SANITIZE TOUPPER)
	cmake_parse_arguments(SEL4M_STRING "${options}" "" "" ${ARGN})

	if (NOT SEL4M_STRING_UNPARSED_ARGUMENTS)
		message(FATAL_ERROR "Function sel4m_string() called without a return variable")
	endif()

	list(GET SEL4M_STRING_UNPARSED_ARGUMENTS 0 return_arg)
	list(REMOVE_AT SEL4M_STRING_UNPARSED_ARGUMENTS 0)

	list(JOIN SEL4M_STRING_UNPARSED_ARGUMENTS "" work_string)

	if(SEL4M_STRING_SANITIZE)
		string(REGEX REPLACE "[^a-zA-Z0-9_]" "_" work_string ${work_string})
	endif()

	if(SEL4M_STRING_TOUPPER)
		string(TOUPPER ${work_string} work_string)
	endif()

	set(${return_arg} ${work_string} PARENT_SCOPE)
endfunction()

function(sel4m_check_cache variable)
	cmake_parse_arguments(CACHE_VAR "REQUIRED;WATCH" "" "" ${ARGN})
	string(TOLOWER ${variable} variable_text)
	string(REPLACE "_" " " variable_text ${variable_text})

	get_property(cached_value CACHE ${variable} PROPERTY VALUE)

	# If the build has already been configured in an earlier CMake invocation,
	# then CACHED_${variable} is set. The CACHED_${variable} setting takes
	# precedence over any user or CMakeLists.txt input.
	# If we detect that user tries to change the setting, then print a warning
	# that a pristine build is needed.

	# If user uses -D<variable>=<new_value>, then cli_argument will hold the new
	# value, otherwise cli_argument will hold the existing (old) value.
	set(cli_argument ${cached_value})
	if(cli_argument STREQUAL CACHED_${variable})
		# The is no changes to the <variable> value.
		unset(cli_argument)
	endif()

	set(app_cmake_lists ${${variable}})
	if(cached_value STREQUAL ${variable})
		# The app build scripts did not set a default, The BOARD we are
		# reading is the cached value from the CLI
		unset(app_cmake_lists)
	endif()

	if(DEFINED CACHED_${variable})
		# Warn the user if it looks like he is trying to change the board
		# without cleaning first
		if(cli_argument)
			if(NOT ((CACHED_${variable} STREQUAL cli_argument) OR (${variable}_DEPRECATED STREQUAL cli_argument)))
				message(WARNING "The build directory must be cleaned pristinely when "
												"changing ${variable_text},\n"
												"Current value=\"${CACHED_${variable}}\", "
												"Ignored value=\"${cli_argument}\"")
			endif()
		endif()

		if(CACHED_${variable})
			set(${variable} ${CACHED_${variable}} PARENT_SCOPE)
			# This resets the user provided value with previous (working) value.
			set(${variable} ${CACHED_${variable}} CACHE STRING "Selected ${variable_text}" FORCE)
		else()
			unset(${variable} PARENT_SCOPE)
			unset(${variable} CACHE)
		endif()
	elseif(cli_argument)
		set(${variable} ${cli_argument})

	elseif(DEFINED ENV{${variable}})
		set(${variable} $ENV{${variable}})

	elseif(app_cmake_lists)
		set(${variable} ${app_cmake_lists})

	elseif(${CACHE_VAR_REQUIRED})
		message(FATAL_ERROR "${variable} is not being defined on the CMake command-line in the environment or by the app.")
	endif()

	# Store the specified variable in parent scope and the cache
	set(${variable} ${${variable}} PARENT_SCOPE)
	set(CACHED_${variable} ${${variable}} CACHE STRING "Selected ${variable_text}")

	if(CACHE_VAR_WATCH)
		# The variable is now set to its final value.
		sel4m_boilerplate_watch(${variable})
	endif()
endfunction()

# Usage:
#   sel4m_boilerplate_watch(SOME_BOILERPLATE_VAR)
#
# Inform the build system that SOME_BOILERPLATE_VAR, a variable
# handled in cmake/app/boilerplate.cmake, is now fixed and should no
# longer be changed.
#
# This function uses variable_watch() to print a noisy warning
# if the variable is set after it returns.
function(sel4m_boilerplate_watch variable)
	variable_watch(${variable} sel4m_variable_set_too_late)
endfunction()

function(sel4m_variable_set_too_late variable access value current_list_file)
	if (access STREQUAL "MODIFIED_ACCESS")
		message(WARNING
"
	 **********************************************************************
	 *
	 *                    WARNING
	 *
	 * CMake variable ${variable} set to \"${value}\" in:
	 *     ${current_list_file}
	 *
	 * This is too late to make changes! The change was ignored.
	 *
	 * Hint: ${variable} must be set before calling find_package(sel4m ...).
	 *
	 **********************************************************************
")
	endif()
endfunction()

# Usage:
#   sel4m_get_targets(<directory> <types> <targets>)
#
# Get build targets for a given directory and sub-directories.
#
# This functions will traverse the build tree, starting from <directory>.
# It will read the `BUILDSYSTEM_TARGETS` for each directory in the build tree
# and return the build types matching the <types> list.
# Example of types: OBJECT_LIBRARY, STATIC_LIBRARY, INTERFACE_LIBRARY, UTILITY.
#
# returns a list of targets in <targets> matching the required <types>.
function(sel4m_get_targets directory types targets)
		get_property(sub_directories DIRECTORY ${directory} PROPERTY SUBDIRECTORIES)
		get_property(dir_targets DIRECTORY ${directory} PROPERTY BUILDSYSTEM_TARGETS)
		foreach(dir_target ${dir_targets})
			get_property(target_type TARGET ${dir_target} PROPERTY TYPE)
			if(${target_type} IN_LIST types)
				list(APPEND ${targets} ${dir_target})
			endif()
		endforeach()

		foreach(directory ${sub_directories})
		sel4m_get_targets(${directory} "${types}" ${targets})
		endforeach()
		set(${targets} ${${targets}} PARENT_SCOPE)
endfunction()

# Usage:
#   target_byproducts(TARGET <target> BYPRODUCTS <file> [<file>...])
#
# Specify additional BYPRODUCTS that this target produces.
#
# This function allows the build system to specify additional byproducts to
# target created with `add_executable()`. When linking an executable the linker
# may produce additional files, like map files. Those files are not known to the
# build system. This function makes it possible to describe such additional
# byproducts in an easy manner.
function(target_byproducts)
	cmake_parse_arguments(TB "" "TARGET" "BYPRODUCTS" ${ARGN})

	if(NOT DEFINED TB_TARGET)
		message(FATAL_ERROR "target_byproducts() missing parameter: TARGET <target>")
	endif()

	add_custom_command(TARGET ${TB_TARGET}
										 POST_BUILD COMMAND ${CMAKE_COMMAND} -E echo ""
										 BYPRODUCTS ${TB_BYPRODUCTS}
										 COMMENT "Logical command for additional byproducts on target: ${TB_TARGET}"
	)
endfunction()
