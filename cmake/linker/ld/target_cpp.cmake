# SPDX-License-Identifier: Apache-2.0

# See root CMakeLists.txt for description and expectations of these macros

macro(toolchain_ld_cpp location)

  sel4m_link_libraries(
    -lstdc++
	${location}
  )

endmacro()
