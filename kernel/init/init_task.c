#include <minix_rt/sched.h>
#include <minix_rt/mm_types.h>
#include <minix_rt/sched.h>
#include <minix_rt/sched/rt.h>
#include <minix_rt/stackprotector.h>

struct task_struct idle_threads[CONFIG_NR_CPUS] = {
	[0 ... CONFIG_NR_CPUS - 1] = {
		.thread_info	= INIT_THREAD_INFO(idle_threads),
		.stack_refcount	= ATOMIC_INIT(1),
		.state 			= 0,
		.notifier 		= {
			.notifier_table = NOTIFIER_TABLE_MASK_NONE,
			.action			= { { { .sa_handler = NOTIFIER_DFL, }, }, },
			.blocked		= {{0}},
			.siglock		= __SPIN_LOCK_UNLOCKED(init_sighand.siglock),
		},
		.prio			= MAX_PRIO,
		.static_prio	= MAX_PRIO,
		.normal_prio	= MAX_PRIO,
		.policy			= SCHED_IDLE,
		.time_slice 	= RR_TIMESLICE,
		.mm				= &init_mm,
		.usage			= ATOMIC_INIT(2),
		.parent			= NULL,
	},
};

void __init early_idle_task_init(void)
{
	int cpu;
	struct task_struct *idle;

	process_pid_init();

	for_each_possible_cpu(cpu) {
		idle = &idle_threads[cpu];
		boot_init_stack_canary(idle);
		idle->flags = PF_IDLE;
		idle->cpu = cpu;
		idle->oncpu = 0;
		idle->stack = &kernel_stack_alloc[cpu];
		task_set_stack_end_magic(idle);
		snprintf(idle->comm, TASK_COMM_LEN, "idle-%d", cpu);
		spin_lock_init(&idle->pi_lock);
		INIT_LIST_HEAD(&idle->children);
		INIT_LIST_HEAD(&idle->children_list);
		INIT_LIST_HEAD(&idle->children_exit);
	}
}
