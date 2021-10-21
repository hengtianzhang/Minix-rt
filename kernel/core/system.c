#include <base/init.h>
#include <base/common.h>
#include <base/elf.h>
#include <base/string.h>
#include <base/errno.h>

#include <minix_rt/cpumask.h>
#include <minix_rt/slab.h>
#include <minix_rt/gfp.h>
#include <minix_rt/page.h>
#include <minix_rt/sched.h>
#include <minix_rt/system.h>
#include <minix_rt/sched/rt.h>
#include <minix_rt/mmap.h>
#include <minix_rt/pid.h>
#include <minix_rt/uaccess.h>
#include <minix_rt/delay.h>

#include <asm/processor.h>

static void system_handle_ipc_message(endpoint_t ep, message_t *m)
{
	switch (m->m_type & IPC_M_TYPE_MASK) {
		case IPC_M_TYPE_SYSTEM_BRK:
		case IPC_M_TYPE_SYSTEM_SBRK:
			system_brk(ep, m);
			break;
		case IPC_M_TYPE_SYSTEM_STRING:
			system_string(ep, m);
			break;
		case IPC_M_TYPE_SYSTEM_MMAP:
			system_mmap(ep, m);
			break;
		default:
			system_misc(ep, m);
			break;
	}
}

int system_thread(void *arg)
{
	while (1) {
		int ret;
		message_t m;

		memset(&m, 0, sizeof (message_t));

		ret = __ipc_receive(ENDPOINT_SYSTEM, &m);
		BUG_ON(ret);

		system_handle_ipc_message(ENDPOINT_SYSTEM, &m);

		ret = __ipc_reply(ENDPOINT_SYSTEM, &m);
		BUG_ON(ret);
	}
	BUG();
};

struct task_struct * __init create_system_task(void)
{
	cpumask_t mask;
	int ret;
	struct task_struct *tsk;

	tsk = task_create_tsk(PF_SYSTEMSERVICE | PF_KTHREAD);
	BUG_ON(!tsk);

	strlcpy(tsk->comm, INIT_SERVICE_COMM, sizeof (tsk->comm));

	ret = -ENOMEM;
	tsk->stack = kmalloc(THREAD_SIZE, GFP_KERNEL | GFP_ZERO);
	BUG_ON(!tsk->stack);

	task_set_stack_end_magic(tsk);
	tsk->state = TASK_RUNNING;
	tsk->pid.pid = 1;
	ret = pid_insert_process_by_pid(tsk);

	BUG_ON(ret == false);

	BUG_ON(ipc_register_endpoint_by_tsk(tsk));

	tsk->policy = SCHED_FIFO;
	tsk->prio = 0;
	tsk->static_prio = 0;
	tsk->normal_prio = 0;
	tsk->sched_class = &rt_sched_class;
	tsk->time_slice = RR_TIMESLICE;

	mask = CPU_MASK_ALL;
	set_cpus_allowed(tsk, mask);

	sched_fork(tsk, 0);

	copy_thread(tsk->flags, (unsigned long)system_thread, 0, tsk);

	return tsk;
}

void __init system_task_init(void)
{
	struct task_struct *tsk;

	tsk = create_system_task();
	BUG_ON(!tsk);

	wake_up_new_task(tsk, 0);
}
