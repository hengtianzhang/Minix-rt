# SPDX-License-Identifier: Apache-2.0

if(DEFINED TOOLCHAIN_HOME)
  # When Toolchain home is defined, then we are cross-compiling, so only look
  # for linker in that path, else we are using host tools.
  set(LLD_SEARCH_PATH PATHS ${TOOLCHAIN_HOME} NO_DEFAULT_PATH)
endif()

find_program(CMAKE_LINKER     ld.lld ${LLD_SEARCH_PATH})

set_ifndef(LINKERFLAGPREFIX -Wl)

# Run $LINKER_SCRIPT file through the C preprocessor, producing ${linker_script_gen}
# NOTE: ${linker_script_gen} will be produced at build-time; not at configure-time
macro(configure_linker_script linker_script_gen linker_pass_define location)
  set(extra_dependencies ${ARGN})

  # Different generators deal with depfiles differently.
  if(CMAKE_GENERATOR STREQUAL "Unix Makefiles")
    # Note that the IMPLICIT_DEPENDS option is currently supported only
    # for Makefile generators and will be ignored by other generators.
    set(linker_script_dep IMPLICIT_DEPENDS C ${LINKER_SCRIPT})
  elseif(CMAKE_GENERATOR STREQUAL "Ninja")
    # Using DEPFILE with other generators than Ninja is an error.
    set(linker_script_dep DEPFILE ${linker_script_gen}.dep)
  else()
    # TODO: How would the linker script dependencies work for non-linker
    # script generators.
    message(WARNING "This generator is not well supported. The
                    Linker script may not be regenerated when it should.")
    set(linker_script_dep "")
  endif()

  kernel_get_include_directories_for_lang(C current_includes ${location})
  get_filename_component(base_name ${CMAKE_CURRENT_BINARY_DIR} NAME)
  get_property(current_defines GLOBAL PROPERTY PROPERTY_LINKER_SCRIPT_DEFINES)

  add_custom_command(
    OUTPUT ${linker_script_gen}
    DEPENDS
    ${LINKER_SCRIPT}
    ${extra_dependencies}
    # NB: 'linker_script_dep' will use a keyword that ends 'DEPENDS'
    ${linker_script_dep}
    COMMAND ${CMAKE_C_COMPILER}
    -x assembler-with-cpp
    -undef
    -MD -MF ${linker_script_gen}.dep -MT ${base_name}/${linker_script_gen}
    -D_LINKER
    -D_ASMLANGUAGE
    ${current_includes}
    ${current_defines}
    ${linker_pass_define}
    -E ${LINKER_SCRIPT}
    -P # Prevent generation of debug `#line' directives.
    -o ${linker_script_gen}
		BYPRODUCTS
		${linker_script_gen}.dep
    VERBATIM
    WORKING_DIRECTORY ${APPLICATION_BINARY_DIR}
    COMMAND_EXPAND_LISTS
  )
endmacro()

function(toolchain_ld_link_kernel_elf)
	cmake_parse_arguments(
		TOOLCHAIN_LD_LINK_ELF                                     # prefix of output variables
		""                                                        # list of names of the boolean arguments
		"TARGET_ELF;OUTPUT_MAP;LINKER_SCRIPT"                     # list of names of scalar arguments
		"DEPENDENCIES" # list of names of list arguments
		${ARGN}                                                   # input args to parse
	)

	target_link_libraries(
		${TOOLCHAIN_LD_LINK_ELF_TARGET_ELF}
		${use_linker}
		${TOPT}
		${TOOLCHAIN_LD_LINK_ELF_LINKER_SCRIPT}
		${LINKERFLAGPREFIX},-Map=${TOOLCHAIN_LD_LINK_ELF_OUTPUT_MAP}
		${LINKERFLAGPREFIX},--whole-archive
		${KERNEL_BUILT_IN_LIBS_PROPERTY}
    version
		${LINKERFLAGPREFIX},--no-whole-archive
		${LINKERFLAGPREFIX},--start-group
		${KERNEL_INTERFACE_LIBS_PROPERTY}
		${LINKERFLAGPREFIX},--end-group
		-nostdlib -static
		${TOOLCHAIN_LD_LINK_ELF_DEPENDENCIES}
	)
endfunction()

# Force symbols to be entered in the output file as undefined symbols
function(toolchain_ld_kernel_force_undefined_symbols location)
  foreach(symbol ${ARGN})
    kernel_link_libraries(${LINKERFLAGPREFIX},-u,${symbol} ${location})
  endforeach()
endfunction()
