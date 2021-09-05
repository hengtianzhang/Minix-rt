if(CONFIG_ARM_PSCI_FW)
    kernel_library()
    kernel_library_sources(psci.c)
endif()
