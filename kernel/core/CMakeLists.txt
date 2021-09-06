
kernel_sources(
    stack.c panic.c printf.c extable.c
    cpu.c reboot.c
)

add_subdirectory(locking)
