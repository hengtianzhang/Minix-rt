# SPDX-License-Identifier: Apache-2.0

# This file provides minix_rt Config Package functionality.
#
# The purpose of this files is to allow users to decide if they want to:
# - Use MINIX_RT_BASE environment setting for explicitly set select a minix_rt installation
# - Support automatic minix_rt installation lookup through the use of find_package(minix_rt)

# First check to see if user has provided a minix_rt base manually.
# Set minix_rt base to environment setting.
# It will be empty if not set in environment.

macro(include_boilerplate location)
	set(minix_rt_FOUND True)
	set(BOILERPLATE_FILE ${MINIX_RT_BASE}/cmake/app/boilerplate.cmake)

	if(NOT NO_BOILERPLATE)
    	message("Including boilerplate (${location}): ${BOILERPLATE_FILE}")
    	include(${BOILERPLATE_FILE} NO_POLICY_SCOPE)
	endif()
endmacro()

set(ENV_MINIX_RT_BASE $ENV{MINIX_RT_BASE})
if((NOT DEFINED MINIX_RT_BASE) AND (DEFINED ENV_MINIX_RT_BASE))
	# Get rid of any double folder string before comparison, as example, user provides
	# MINIX_RT_BASE=//path/to//minix_rt_base/
	# must also work.
	get_filename_component(MINIX_RT_BASE ${ENV_MINIX_RT_BASE} ABSOLUTE)
	set(MINIX_RT_BASE ${MINIX_RT_BASE} CACHE PATH "minix_rt base")
	include_boilerplate("minix_rt base")
	return()
endif()

if(DEFINED MINIX_RT_BASE)
	include_boilerplate("minix_rt base (cached)")
	return()
endif()

message(FATAL_ERROR "Minix-rt cmake Can't have gotten here.")
