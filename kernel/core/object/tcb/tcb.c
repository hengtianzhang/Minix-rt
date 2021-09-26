#include <sel4m/sched.h>
#include <sel4m/slab.h>
#include <sel4m/gfp.h>
#include <sel4m/object/tcb.h>
#include <sel4m/object/pid.h>
#include <sel4m/object/untype.h>
#include <sel4m/syscalls.h>

#include <uapi/sel4m/object/tcb.h>

asmlinkage void ret_from_fork(void) asm("ret_from_fork");

struct task_struct *tcb_create_task(unsigned int flags)
{
	struct task_struct *tsk;
	struct mm_struct *mm;

	tsk = kmalloc(sizeof (*tsk), GFP_KERNEL | GFP_ZERO);
	if (!tsk)
		return NULL;

	mm = untype_alloc_mm_struct();
	if (!mm)
		goto fail_mm;

	tsk->mm = mm;

	tsk->flags = flags;
	tsk->state = TASK_NEW;
	tsk->oncpu = 0;
	atomic_set(&tsk->usage, 2);
	spin_lock_init(&tsk->pi_lock);
	tsk->parent = NULL;
	INIT_LIST_HEAD(&tsk->children);
	INIT_LIST_HEAD(&tsk->children_list);
	INIT_LIST_HEAD(&tsk->children_exit);
	notifier_table_clearall(&tsk->notifier.notifier_table);
	memset(tsk->notifier.action, 0, sizeof (struct k_sigaction));

	return tsk;

fail_mm:
	kfree(tsk);

	return NULL;
}

void tcb_destroy_task(struct task_struct *tsk)
{
	if (!tsk)
		return;

	untype_free_mm_struct(tsk->mm);
	kfree(tsk);
}

void tcb_set_task_stack_end_magic(struct task_struct *tsk)
{
	unsigned long *stackend;

	stackend = end_of_stack(tsk);
	*stackend = STACK_END_MAGIC;	/* for overflow detection */
}

static pid_t do_fork(unsigned long ventry, unsigned long varg,
				unsigned long clone_flags, unsigned long return_fn)
{
	int ret;
	struct pt_regs *regs;
	struct task_struct *tsk;
	unsigned long stack_top, ipcptr;

	if (BAD_ADDR(ventry))
		goto fail_ventry;

	if (BAD_ADDR(varg))
		goto fail_ventry;

	if (BAD_ADDR(return_fn))
		goto fail_ventry;

	tsk = tcb_create_task(clone_flags);
	if (!tsk)
		return -ENOMEM;

	ret = pid_alloc_pid(tsk);
	if (ret)
		goto fail_inster_pid;

	snprintf(tsk->comm, TASK_COMM_LEN, "%s-%d", current->comm, tsk->pid.pid);
	tsk->stack = kmalloc(THREAD_SIZE, GFP_KERNEL | GFP_ZERO);
	if (!tsk->stack)
		goto fail_stack;

	tcb_set_task_stack_end_magic(tsk);

	ret = untype_copy_mm(tsk, current, &stack_top, &ipcptr);
	if (ret)
		goto fail_copy_mm;

	tsk->flags |= current->flags;
	tsk->cap_ipcptr = (void *)ipcptr;
	tsk->parent = current;

	list_add(&tsk->children_list, &current->children);

	task_thread_info(tsk)->addr_limit = USER_DS;

	tsk->state = TASK_RUNNING;

	tsk->policy = current->policy;
	tsk->prio = current->prio;
	tsk->static_prio = current->static_prio;
	tsk->normal_prio = current->normal_prio;
	tsk->sched_class = current->sched_class;
	tsk->time_slice = current->time_slice;
	set_cpus_allowed(tsk, current->cpus_allowed);

	sched_fork(tsk, 0);

	regs = task_pt_regs(tsk);
	start_thread(regs, ventry, stack_top);
	regs->regs[0] = varg;
	regs->regs[30] = return_fn;

	memset(&tsk->thread.cpu_context, 0, sizeof(struct cpu_context));

	tsk->thread.cpu_context.pc = (unsigned long)ret_from_fork;
	tsk->thread.cpu_context.sp = (unsigned long)regs;

	wake_up_new_task(tsk, 0);

	return tsk->pid.pid;

fail_copy_mm:
	kfree(tsk->stack);
fail_stack:
	ret = pid_remove_pid_by_process(tsk);
	BUG_ON(!ret);
fail_inster_pid:
	tcb_destroy_task(tsk);
fail_ventry:

	return -EINVAL;
}

SYSCALL_DEFINE4(tcb_thread, enum tcb_table, table, unsigned long, fn,
				unsigned long, arg, unsigned long, return_fn)
{
	if (!cap_table_test_cap(cap_thread_cap, &current->cap_table))
		return -ENOTCB;

	switch (table) {
		case tcb_create_thread_fn:
			return do_fork(fn, arg, PF_THREAD, return_fn);
		case tcb_create_tcb_object:
			printf("tcb_create_tcb_object Nothing TODO!\n");
			return -ECHILD;
		case tcb_clone_task_fn:
			printf("tcb_create_tcb_object Nothing TODO!\n");
			return -ECHILD;
	}

	return -EINVAL;;
}
