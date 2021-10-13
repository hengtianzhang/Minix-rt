
add_executable(vfs)
target_sources(vfs PRIVATE src/main.c)

DeclareServicesApp(vfs)
