#include <sel4m/sched.h>
#include <sel4m/mm_types.h>
#include <sel4m/sched.h>
#include <sel4m/stackprotector.h>

struct task_struct idle_threads[CONFIG_NR_CPUS] = {
	[0 ... CONFIG_NR_CPUS - 1] = {
		.thread_info	= INIT_THREAD_INFO(idle_threads),
		.state 			= 0,
		.prio			= MAX_PRIO,
		.static_prio	= MAX_PRIO,
		.normal_prio	= MAX_PRIO,
		.policy			= SCHED_IDLE,
		.mm 			= &init_mm,
		.usage			= ATOMIC_INIT(2),
	},
};

void early_idle_task_init(void)
{
	int cpu;
	struct task_struct *idle;

	process_pid_init();

	for_each_possible_cpu(cpu) {
		idle = &idle_threads[cpu];
		boot_init_stack_canary(idle);
		idle->cpu = cpu;
		idle->oncpu = 0;
		idle->stack = &kernel_stack_alloc[cpu];
		snprintf(idle->comm, TASK_COMM_LEN, "idle-%d", cpu);
		spin_lock_init(&idle->pi_lock);
	}
}
