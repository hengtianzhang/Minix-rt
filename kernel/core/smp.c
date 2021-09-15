/*
 * Generic helpers for smp ipi calls
 *
 * (C) Jens Axboe <jens.axboe@oracle.com> 2008
 */

#include <base/init.h>

#include <sel4m/smp.h>
#include <sel4m/sched.h>
#include <sel4m/sched/idle.h>

static __init int bringup_cpu(unsigned int cpu)
{
	struct task_struct *idle = &idle_threads[cpu];
	int ret;

	init_idle(idle, cpu);

	/* Arch-specific enabling code. */
	ret = __cpu_up(cpu, idle);

	return ret;
}

static __init int do_cpu_up(unsigned int cpu)
{
	int err;

	if (!cpu_possible(cpu)) {
		printf("can't online cpu %d because it is not configured as may-hotadd at boot time\n",
			   cpu);

		return -EINVAL;
	}

	err = bringup_cpu(cpu);

	return err;
}

/* Called by boot processor to activate the rest. */
void __init smp_init(void)
{
	int num_cpus;
	unsigned int cpu = smp_processor_id();

	printf("Bringing up secondary CPUs ...\n");

	/* FIXME: This should be done in userspace --RR */
	for_each_present_cpu(cpu) {
		if (num_online_cpus() >= CONFIG_NR_CPUS)
			break;
		if (!cpu_online(cpu))
			do_cpu_up(cpu);
	}

	num_cpus  = num_online_cpus();
	printf("Brought up %d CPU%s\n",
		num_cpus,  (num_cpus  > 1 ? "s" : ""));

	/* Any cleanup work */
	smp_cpus_done(CONFIG_NR_CPUS);
}
