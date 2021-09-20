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
#include <sel4m/of_fdt.h>
#include <sel4m/extable.h>
#include <sel4m/irq.h>
#include <sel4m/ktime.h>
#include <sel4m/hrtimer.h>
#include <sel4m/sched.h>
#include <sel4m/gfp.h>
#include <sel4m/object/untype.h>
#include <sel4m/sched/idle.h>

enum system_states system_state __read_mostly;

extern const char linux_banner[];

void __weak __init early_arch_platform_init(void) {}
void __weak __init setup_arch(void) {}

noinline void rest_init(void)
{
	struct task_struct *tsk;

	system_state = SYSTEM_SCHEDULING;

	smp_prepare_cpus(CONFIG_NR_CPUS);

	smp_init();

	sched_init_smp();

	untype_core_init();

	tsk = service_core_init();
	BUG_ON(!tsk);

	untype_core_init_late(tsk);

	mark_rodata_ro();

	system_state = SYSTEM_RUNNING;

	printf("Booting all finished, dropped to user space\n");

	wake_up_new_task(tsk, 0);
	preempt_enable_no_resched();
	schedule();
	preempt_disable();

	cpu_idle();
}

asmlinkage __visible void __init start_kernel(void)
{
	system_state = SYSTEM_BOOTING;

	smp_setup_processor_id();

	local_irq_disable();

	boot_cpu_init();
	early_arch_platform_init();

	early_idle_task_init();	

	printf("%s", linux_banner);
	setup_arch();

	smp_prepare_boot_cpu();

	printf("Kernel command line: %s\n", boot_command_line);

	sort_main_extable();

	/*
	 * Set up the scheduler prior starting any interrupts (such as the
	 * timer interrupt). Full topology setup happens at smp_init()
	 * time - but meanwhile we still have a functioning scheduler.
	 */
	sched_init();

	call_function_init();

	/*
	 * Disable preemption - early bootup scheduling is extremely
	 * fragile until we cpu_idle() for the first time.
	 */
	preempt_disable();
	if (WARN(!irqs_disabled(),
		 "Interrupts were enabled *very* early, fixing it\n"))
		local_irq_disable();

	init_IRQ();
	time_init();
	hrtimers_init();

	WARN(!irqs_disabled(), "Interrupts were enabled early\n");
	local_irq_enable();

	sched_clock_init();
	system_tick_init();

	/* Do the rest non-__init'ed, we're now alive */
	rest_init();
}
