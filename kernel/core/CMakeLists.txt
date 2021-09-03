
kernel_sources(
    stack.c panic.c printf.c
    cpu.c
)

add_subdirectory(locking)
