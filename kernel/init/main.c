/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org>
 */
#include <base/init.h>

#include <sel4m/cpu.h>
#include <sel4m/irqflags.h>
#include <sel4m/smp.h>
#include <sel4m/stackprotector.h>
#include <sel4m/of_fdt.h>
#include <sel4m/extable.h>

extern const char linux_banner[];

void __weak __init early_arch_platform_init(void) {}
void __weak __init setup_arch(void) {}

asmlinkage __visible void __init start_kernel(void)
{
    smp_setup_processor_id();

    local_irq_disable();

    boot_cpu_init();
    early_arch_platform_init();

    printf("%s", linux_banner);
    setup_arch();

	boot_init_stack_canary();

	smp_prepare_boot_cpu();

    printf("Kernel command line: %s\n", boot_command_line);

    sort_main_extable();

    hang ("This is Stop!\n");
}
