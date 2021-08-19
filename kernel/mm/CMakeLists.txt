
kernel_library()

kernel_library_sources(test1.c)

find_package(misclibc REQUIRED)

misclibc_kernel_import_libraries()
