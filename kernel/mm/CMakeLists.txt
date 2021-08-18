
kernel_library()

kernel_library_sources(test1.c)

find_package(musllibc REQUIRED)

musllibc_kernel_import_libraries()
