
kernel_interface_library()

kernel_interface_library_sources(
    vsprintf.c ctype.c string.c div64.c
)

find_package(memblock REQUIRED)
memblock_kernel_import_libraries()
