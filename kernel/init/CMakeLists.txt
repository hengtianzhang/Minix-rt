
add_library(version version.c)

set_target_properties(
	version
	PROPERTIES
  	COMPILE_DEFINITIONS
	__KERNEL_VERSION__
)

add_dependencies(version ${COMPILE_H_TARGET})

target_link_libraries(version kernel_interface)

kernel_sources(main.c)
