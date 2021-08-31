
kernel_interface_library()

kernel_interface_library_sources(
    test.c
)

find_package(memalloc REQUIRED)
memalloc_kernel_import_libraries()
