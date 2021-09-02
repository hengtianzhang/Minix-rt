
target_sources(${of_VAR}of
	PRIVATE
	fdt.c fdt_ro.c fdt_wip.c fdt_rw.c
	fdt_sw.c fdt_strerror.c fdt_empty_tree.c
)

target_include_directories(${of_VAR}of
	PRIVATE
	libfdt
)
