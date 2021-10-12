
kernel_sources(
    stack.c panic.c printf.c extable.c
    cpu.c reboot.c clock.c softirq.c
    smp.c exit.c syscall.c signal.c
    pid.c fork.c
)

add_subdirectory(irq)
add_subdirectory(locking)
add_subdirectory(object)
add_subdirectory(sched)
add_subdirectory(time)
