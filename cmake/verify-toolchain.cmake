# The purpose of this file is to verify that required variables has been
# defined for proper toolchain use.

# Set internal variables if set in environment.
if(NOT DEFINED MINIX_RT_TOOLCHAIN)
  set(MINIX_RT_TOOLCHAIN $ENV{MINIX_RT_TOOLCHAIN})
endif()

if(NOT MINIX_RT_TOOLCHAIN AND
   (CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE})))
    set(MINIX_RT_TOOLCHAIN cross-compile)
endif()

if(NOT DEFINED MINIX_RT_TOOLCHAIN)
	message(FATAL_ERROR "No toolchain defining the available tools, The sample:
	MINIX_RT_TOOLCHAIN=llvm or CROSS_COMPILE
")
endif()
