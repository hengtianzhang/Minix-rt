
target_sources(${of_VAR}of
	PRIVATE
	base.c property.c address.c
	fdt.c fdt_address.c irq.c
)

add_subdirectory(fdt)
