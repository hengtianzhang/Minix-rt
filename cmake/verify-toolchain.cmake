# The purpose of this file is to verify that required variables has been
# defined for proper toolchain use.

# Set internal variables if set in environment.
if(NOT DEFINED SEL4M_TOOLCHAIN)
  set(SEL4M_TOOLCHAIN $ENV{SEL4M_TOOLCHAIN})
endif()

if(NOT SEL4M_TOOLCHAIN AND
   (CROSS_COMPILE OR (DEFINED ENV{CROSS_COMPILE})))
    set(SEL4M_TOOLCHAIN cross-compile)
endif()

if(NOT DEFINED SEL4M_TOOLCHAIN)
	message(FATAL_ERROR "No toolchain defining the available tools, The sample:
	SEL4M_TOOLCHAIN=llvm or CROSS_COMPILE
")
endif()
