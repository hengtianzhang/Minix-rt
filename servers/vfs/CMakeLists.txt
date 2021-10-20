
add_executable(vfs)
add_subdirectory(src)

find_package(cpio REQUIRED)
cpio_user_import_libraries()

target_link_libraries(vfs PRIVATE cpio)

DeclareServicesApp(vfs)
