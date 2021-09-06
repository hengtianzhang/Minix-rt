
if(CONFIG_TIMER_OF)
	kernel_library()
	kernel_library_sources(timer-of.c)

	kernel_library_sources_ifdef(CONFIG_TIMER_PROBE timer-probe.c)
	kernel_library_sources_ifdef(CONFIG_ARM_ARCH_TIMER arm_arch_timer.c)
endif()
