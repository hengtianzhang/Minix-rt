
kernel_interface_library()

kernel_interface_library_sources(test2.c)

find_package(musllibc REQUIRED)

musllibc_kernel_import_libraries()

find_package(misclibc REQUIRED)

misclibc_kernel_import_libraries()

misclibc_import_libraries()
misclibc_import_libraries()
misclibc_kernel_import_libraries()

musllibc_kernel_import_libraries()
