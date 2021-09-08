
kernel_sources(
    stack.c panic.c printf.c extable.c
    cpu.c reboot.c clock.c softirq.c
)

add_subdirectory(locking)
add_subdirectory(irq)
add_subdirectory(time)
add_subdirectory(sched)
