# SPDX-License-Identifier: Apache-2.0

if(NOT DEFINED KERNEL_BINARY_DIR)
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
if(NOT ${SEL4M_BASE}/kernel STREQUAL CMAKE_CURRENT_SOURCE_DIR)
message(WARNING "kernel doesn't match CMAKE_CURRENT_SOURCE_DIR
	SEL4M_BASE              = ${SEL4M_BASE}
	PWD                      = $ENV{PWD}
	CMAKE_CURRENT_SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}
You may be using a mix of symbolic links and real paths which causes \
subtle and hard to debug CMake issues.")
endif()

# Verify that the toolchain can compile a dummy file, if it is not we
# won't be able to test for compatibility with certain C flags.
sel4m_check_compiler_flag(C "" toolchain_is_ok)
assert(toolchain_is_ok "The toolchain is unable to build a dummy C file. See CMakeError.log.")

project(kernel VERSION ${PROJECT_VERSION})
enable_language(C CXX ASM)

# In some cases the "final" things are not used at all and "_prebuilt"
# is the last station. See "logical_target_for_kernel_elf" below for
# details.
set(CMAKE_EXECUTABLE_SUFFIX .elf)
set(KERNEL_FINAL_EXECUTABLE    kernel_final)

# Set some phony targets to collect dependencies
set(ASM_OFFSETS_H_TARGET           asm_offsets_h)

add_library(kernel_interface INTERFACE)

kernel_library_named(kernel)

kernel_include_directories(
	include
	arch/${ARCH}/include
	${APPLICATION_BINARY_DIR}/include
)

kernel_compile_definitions(
	__KERNEL__
	__SEL4M__=1
)

# Common toolchain-agnostic assembly flags
kernel_compile_options(
	$<$<COMPILE_LANGUAGE:ASM>:-D__ASSEMBLY__>
)

if(BUILD_VERSION)
	kernel_compile_definitions(
		BUILD_VERSION=${BUILD_VERSION}
	)
endif()

kernel_compile_options($<$<COMPILE_LANGUAGE:C>:
	-Wundef -Werror=strict-prototypes -Wno-trigraphs
	-fno-strict-aliasing -fshort-wchar
	-Werror-implicit-function-declaration
	-Wno-format-security
	>
)

if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
execute_process(
	COMMAND which
	${CMAKE_LINKER}
	OUTPUT_VARIABLE GCC_TOOLCHAIN_DIR
)
get_filename_component(GCC_TOOLCHAIN_DIR ${GCC_TOOLCHAIN_DIR} DIRECTORY)
kernel_cc_option(--prefix=${GCC_TOOLCHAIN_DIR})

get_filename_component(SECOND_FOLDER ${GCC_TOOLCHAIN_DIR} DIRECTORY)
kernel_cc_option(--gcc-toolchain=${SECOND_FOLDER})

kernel_cc_option(-no-integrated-as)
endif()

kernel_cc_option(-fno-delete-null-pointer-checks)
kernel_cc_option(-Wno-frame-address)
kernel_cc_option(-Wno-format-truncation)
kernel_cc_option(-Wno-format-overflow)
kernel_cc_option(-Wno-int-in-bool-context)

if(CONFIG_CC_OPTIMIZE_FOR_SIZE)
kernel_cc_option_fallback(-Oz, -Os)
else()
kernel_cc_option(-O2)
endif()

kernel_cc_option(-Wno-maybe-uninitialized)

# Tell gcc to never replace conditional load with a non-conditional one
kernel_cc_option(--param=allow-store-data-races=0)

kernel_cc_option_ifdef(CONFIG_FRAME_WARN -Wframe-larger-than=${CONFIG_FRAME_WARN})

kernel_cc_option_ifdef(CONFIG_CC_HAS_STACKPROTECTOR_NONE -fno-stack-protector)
kernel_cc_option_ifdef(CONFIG_STACKPROTECTOR -fstack-protector)
kernel_cc_option_ifdef(CONFIG_STACKPROTECTOR_STRONG -fstack-protector-strong)

if(CONFIG_CC_IS_CLANG)
kernel_cc_option(-Qunused-arguments)
# CLANG uses a _MergedGlobals as optimization, but this breaks modpost, as the
# source of a reference will be _MergedGlobals and not on of the whitelisted names.
# See modpost pattern 2
kernel_cc_option(-mno-global-merge)
kernel_cc_option(-fcatch-undefined-behavior)
endif()

kernel_cc_option(-Wno-unused-const-variable)

if(CONFIG_FRAME_POINTER)
kernel_cc_option(-fno-omit-frame-pointer)
kernel_cc_option(-fno-optimize-sibling-calls)
else()
kernel_cc_option(-fomit-frame-pointer)
endif()

kernel_cc_option(-fno-var-tracking-assignments)

if(CONFIG_DEBUG_INFO)
if(CONFIG_DEBUG_INFO_SPLIT)
kernel_cc_option_fallback(-gsplit-dwarf, -g)
else()
kernel_compile_options(-g)
endif()
kernel_compile_options(
	$<$<COMPILE_LANGUAGE:ASM>:-Wa,-gdwarf-2>
)
endif()

kernel_cc_option_ifdef(CONFIG_DEBUG_INFO_DWARF4 -gdwarf-4)

kernel_cc_option_ifdef(CONFIG_DEBUG_INFO_REDUCED -femit-struct-debug-baseonly -fno-var-tracking)

# We trigger additional mismatches with less inlining
kernel_cc_option_ifdef(CONFIG_DEBUG_SECTION_MISMATCH -fno-inline-functions-called-once)

kernel_cc_option_ifdef(CONFIG_LD_DEAD_CODE_DATA_ELIMINATION -ffunction-sections -fdata-sections)

kernel_ld_option_ifdef(CONFIG_LD_DEAD_CODE_DATA_ELIMINATION -Wl,--gc-sections)

kernel_compile_options($<TARGET_PROPERTY:compiler,nostdinc>)
target_include_directories(kernel_interface SYSTEM INTERFACE $<TARGET_PROPERTY:compiler,nostdinc_include>)

# warn about C99 declaration after statement
kernel_compile_options(-Wdeclaration-after-statement)

# Variable Length Arrays (VLAs) should not be used anywhere in the kernel
kernel_cc_option(-Wvla)

# disable stringop warnings in gcc 8+
kernel_cc_option(-Wno-stringop-truncation)

# disable invalid "can't wrap" optimizations for signed / pointers
kernel_cc_option(-fno-strict-overflow)

# clang sets -fmerge-all-constants by default as optimization, but this
# is non-conforming behavior for C and in fact breaks the kernel, so we
# need to disable it here generally.
kernel_cc_option(-fno-merge-all-constants)

# for gcc -fno-merge-all-constants disables everything, but it is fine
# to have actual conforming behavior enabled.
kernel_cc_option(-fmerge-constants)

# Make sure -fstack-check isn't enabled (like gentoo apparently did)
kernel_cc_option(-fno-stack-check)

# conserve stack if available
kernel_cc_option(-fconserve-stack)

# Prohibit date/time macros, which would make the build non-deterministic
kernel_cc_option(-Werror=date-time)

# enforce correct pointer usage
kernel_cc_option(-Werror=incompatible-pointer-types)

# Require designated initializers for all marked structures
kernel_cc_option(-Werror=designated-init)

kernel_cc_option(-fmacro-prefix-map=${CMAKE_SOURCE_DIR}=CMAKE_SOURCE_DIR)
kernel_cc_option(-fmacro-prefix-map=${SEL4M_BASE}=SEL4M_BASE)

# ==========================================================================
#
# cmake -DW=... settings
#
# W=1 - warnings that may be relevant and does not occur too often
# W=2 - warnings that occur quite often but may still be relevant
# W=3 - the more obscure warnings, can most likely be ignored
# ==========================================================================
# @Intent: Add cmake -DW toolchain supported warnings, if any
if(W MATCHES "1")
	kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_1>>)
	kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_1>>)
endif()

if(W MATCHES "2")
	kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_2>>)
	kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_2>>)
endif()

if(W MATCHES "3")
	kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_dw_3>>)
	kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_dw_3>>)
endif()

kernel_ld_options(--build-id)

kernel_ld_option_ifdef(CONFIG_STRIP_ASM_SYMS -X)

kernel_ld_option_ifdef(CONFIG_LD_ORPHAN_WARN --orphan-handling=warn)

# @Intent: Obtain compiler specific flags related to C++ that are not influenced by kconfig
kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,required>>)

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

	kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:${STD_CPP_DIALECT_FLAGS}>)
endif()

kernel_compile_options("SHELL: $<TARGET_PROPERTY:compiler,imacros> ${AUTOCONF_H}")

# @Intent: Set compiler specific flag for bare metal freestanding option
kernel_compile_options($<TARGET_PROPERTY:compiler,freestanding>)

# @Intent: Set compiler specific flag for tentative definitions, no-common
kernel_compile_options($<TARGET_PROPERTY:compiler,no_common>)

# @Intent: Obtain compiler specific flags related to assembly
# ToDo: Remember to get feedback from Oticon on this, as they might use the `ASM_BASE_FLAG` since this is done this way.
kernel_compile_options($<$<COMPILE_LANGUAGE:ASM>:$<TARGET_PROPERTY:asm,required>>)

# @Intent: Add the basic toolchain warning flags
kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_base>>)
kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_base>>)

# @Intent: Add extended, more specific, toolchain warning flags
kernel_compile_options($<TARGET_PROPERTY:compiler,warning_extended>)

# @Intent: Trigger an error when a declaration does not specify a type
kernel_compile_options($<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,warning_error_implicit_int>>)
kernel_compile_options($<$<COMPILE_LANGUAGE:CXX>:$<TARGET_PROPERTY:compiler-cpp,warning_error_implicit_int>>)

# Allow the user to inject options when calling cmake, e.g.
# 'cmake -DEXTRA_CFLAGS="-Werror -Wno-deprecated-declarations" ..'
include(${SEL4M_BASE}/cmake/kernel_extra_flags.cmake)

kernel_cc_option(-fno-asynchronous-unwind-tables)
kernel_cc_option(-fno-PIE)
kernel_cc_option(-fno-strict-overflow)

get_property(CSTD GLOBAL PROPERTY CSTD)
set_ifndef(CSTD gnu89)

# @Intent: Obtain compiler specific flag for specifying the c standard
kernel_compile_options(
	$<$<COMPILE_LANGUAGE:C>:$<TARGET_PROPERTY:compiler,cstd>${CSTD}>
)

get_property(TOPT GLOBAL PROPERTY TOPT)

set_ifndef(  TOPT -Wl,-T) # clang doesn't pick -T for some reason and complains,
													# while -Wl,-T works for both, gcc and clang

set(ASM_OFFSETS_LIB asm_offsets)

set(ASM_OFFSETS_C_PATH ${KERNEL_DIR}/arch/${ARCH}/core/asm-offsets.c)
set(ASM_OFFSETS_H_PATH ${APPLICATION_BINARY_DIR}/include/generated/asm-offsets.h)

add_library(${ASM_OFFSETS_LIB} OBJECT ${ASM_OFFSETS_C_PATH})
target_link_libraries(${ASM_OFFSETS_LIB} kernel_interface)
set_target_properties(${ASM_OFFSETS_LIB} PROPERTIES COMPILE_OPTIONS "-S")

add_custom_command(
	OUTPUT ${ASM_OFFSETS_H_PATH}
	COMMAND ${PYTHON_EXECUTABLE} ${SEL4M_BASE}/scripts/gen_header.py
	-i $<TARGET_OBJECTS:${ASM_OFFSETS_LIB}>
	-o ${ASM_OFFSETS_H_PATH}
	DEPENDS
	${ASM_OFFSETS_LIB}
	$<TARGET_OBJECTS:${ASM_OFFSETS_LIB}>
)
add_custom_target(${ASM_OFFSETS_H_TARGET} DEPENDS ${ASM_OFFSETS_H_PATH})

add_subdirectory(arch/${ARCH})

add_custom_target(kernel_generated_headers)
add_dependencies(kernel_generated_headers
	asm_offsets_h
)

# Read list content
get_property(KERNEL_BUILT_IN_LIBS_PROPERTY GLOBAL PROPERTY KERNEL_BUILT_IN_LIBS)
# Read list content
get_property(KERNEL_INTERFACE_LIBS_PROPERTY GLOBAL PROPERTY KERNEL_INTERFACE_LIBS)

foreach(kernel_built_in_lib ${KERNEL_BUILT_IN_LIBS_PROPERTY})
  # TODO: Could this become an INTERFACE property of zephyr_interface?
  add_dependencies(${kernel_built_in_lib} kernel_generated_headers)
endforeach()

get_property(LINKER_SCRIPT GLOBAL PROPERTY LINKER_SCRIPT_S)
if(NOT EXISTS ${LINKER_SCRIPT})
	message(FATAL_ERROR "Could not find linker script: '${CMAKE_CURRENT_SOURCE_DIR}'. ${CMAKE_CURRENT_LIST_FILE} Corrupted configuration?")
endif()

kernel_get_include_directories_for_lang(C KERNEL_INCLUDE_DIRS STRIP_PREFIX)

configure_linker_script(
	${PROJECT_BINARY_DIR}/linker.lds
	"-D__ASSEMBLY__ -DLINKER_SCRIPT -U${ARCH}"
	kernel_generated_headers
)

add_custom_target(
	linker_kernel_prebuilt_script_target
	DEPENDS
	${PROJECT_BINARY_DIR}/linker.lds
)

set_property(TARGET
	linker_kernel_prebuilt_script_target
	PROPERTY INCLUDE_DIRECTORIES
	${KERNEL_INCLUDE_DIRS}
)

get_property(HEAD_S GLOBAL PROPERTY KERNEL_HEAD_S)
if(NOT EXISTS ${HEAD_S})
	message("Could not find header file: HEAD_S")
endif()

add_executable(${KERNEL_FINAL_EXECUTABLE} ${HEAD_S})
toolchain_ld_link_kernel_elf(
	TARGET_ELF            ${KERNEL_FINAL_EXECUTABLE}
	OUTPUT_MAP            ${PROJECT_BINARY_DIR}/${KERNEL_FINAL_EXECUTABLE}.map
	LINKER_SCRIPT         ${PROJECT_BINARY_DIR}/linker.lds
)
set_property(TARGET   ${KERNEL_FINAL_EXECUTABLE} PROPERTY LINK_DEPENDS ${PROJECT_BINARY_DIR}/linker.lds)
add_dependencies(${KERNEL_FINAL_EXECUTABLE} linker_kernel_prebuilt_script_target)

kernel_library_sources(init/main.c init/test.S)
