# SPDX-License-Identifier: Apache-2.0

# This file provides sel4m Config Package functionality.
#
# The purpose of this files is to allow users to decide if they want to:
# - Use SEL4M_BASE environment setting for explicitly set select a sel4m installation
# - Support automatic sel4m installation lookup through the use of find_package(sel4m)

# First check to see if user has provided a sel4m base manually.
# Set sel4m base to environment setting.
# It will be empty if not set in environment.

macro(include_boilerplate location)
	set(sel4m_FOUND True)
	set(BOILERPLATE_FILE ${SEL4M_BASE}/cmake/app/boilerplate.cmake)

	if(NOT NO_BOILERPLATE)
    	message("Including boilerplate (${location}): ${BOILERPLATE_FILE}")
    	include(${BOILERPLATE_FILE} NO_POLICY_SCOPE)
	endif()
endmacro()

set(ENV_SEL4M_BASE $ENV{SEL4M_BASE})
if((NOT DEFINED SEL4M_BASE) AND (DEFINED ENV_SEL4M_BASE))
	# Get rid of any double folder string before comparison, as example, user provides
	# SEL4M_BASE=//path/to//sel4m_base/
	# must also work.
	get_filename_component(SEL4M_BASE ${ENV_SEL4M_BASE} ABSOLUTE)
	set(SEL4M_BASE ${SEL4M_BASE} CACHE PATH "sel4m base")
	include_boilerplate("sel4m base")
	return()
endif()

if(DEFINED SEL4M_BASE)
	include_boilerplate("sel4m base (cached)")
	return()
endif()

message(FATAL_ERROR "Sel4m cmake Can't have gotten here.")
