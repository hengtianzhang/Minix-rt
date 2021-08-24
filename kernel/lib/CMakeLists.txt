
kernel_interface_library()

kernel_interface_library_sources(test2.c)

find_package(misc REQUIRED)

misc_kernel_import_libraries()
