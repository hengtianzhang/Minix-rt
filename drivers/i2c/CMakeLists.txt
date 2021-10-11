
add_executable(i2c)
target_sources(i2c PRIVATE src/i2c.c)

DeclareDriversApp(i2c)
