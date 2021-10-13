
add_executable(i2c)
target_sources(i2c PRIVATE src/main.c)

DeclareDriversApp(i2c)
