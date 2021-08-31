
kernel_interface_library()

kernel_interface_library_sources(
    test.c
)

find_package(memblock REQUIRED)
memblock_kernel_import_libraries()
