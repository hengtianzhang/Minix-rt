#include <sel4m/sched.h>
#include <sel4m/mm_types.h>
#include <sel4m/sched.h>
#include <sel4m/sched/rt.h>
#include <sel4m/stackprotector.h>
#include <sel4m/object/tcb.h>

#include <uapi/sel4m/object/cap_types.h>

struct task_struct idle_threads[CONFIG_NR_CPUS] = {
	[0 ... CONFIG_NR_CPUS - 1] = {
		.thread_info	= INIT_THREAD_INFO(idle_threads),
		.stack_refcount	= ATOMIC_INIT(1),
		.state 			= 0,
		.cap_table		= CAP_TABLE_MASK_NONE,
		.notifier 		= {
			.notifier_table = NOTIFIER_TABLE_MASK_NONE,
			.action			= { { { .sa_handler = NOTIFIER_DFL, }, }, },
		},
		.prio			= MAX_PRIO,
		.static_prio	= MAX_PRIO,
		.normal_prio	= MAX_PRIO,
		.policy			= SCHED_IDLE,
		.time_slice 	= RR_TIMESLICE,
		.mm 			= &init_mm,
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
		tcb_set_task_stack_end_magic(idle);
		snprintf(idle->comm, TASK_COMM_LEN, "idle-%d", cpu);
		spin_lock_init(&idle->pi_lock);
		INIT_LIST_HEAD(&idle->children);
		INIT_LIST_HEAD(&idle->sibling);
		INIT_LIST_HEAD(&idle->children_list);
		INIT_LIST_HEAD(&idle->sibling_list);
	}
}
