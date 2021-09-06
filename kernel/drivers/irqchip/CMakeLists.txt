
if(CONFIG_IRQCHIP)
	kernel_library()
	kernel_library_sources(irqchip.c)
	kernel_library_sources_ifdef(CONFIG_ARM_GIC
		irq-gic.c irq-gic-common.c
	)
endif()
