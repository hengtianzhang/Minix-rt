
kernel_interface_library()

kernel_interface_library_sources(
    cpumask.c extable.c timerqueue.c reciprocal_div.c
)

kernel_interface_library_sources_ifdef(
    CONFIG_MEMTEST
    memtest.c
)

find_package(memalloc REQUIRED)
memalloc_kernel_import_libraries()
