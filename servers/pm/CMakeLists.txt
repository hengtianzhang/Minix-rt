
add_executable(pm)
target_sources(pm PRIVATE src/main.c)

find_package(libminix_rt REQUIRED)
libminix_rt_user_import_libraries()
target_link_libraries(pm PRIVATE libminix_rt)

DeclareRootservices(pm)
