
add_executable(fs)
target_sources(fs PRIVATE src/main.c)

find_package(libminix_rt REQUIRED)
libminix_rt_user_import_libraries()
target_link_libraries(fs PRIVATE libminix_rt)

DeclareServicesApp(fs)
