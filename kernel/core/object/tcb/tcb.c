#include <sel4m/sched.h>
#include <sel4m/slab.h>
#include <sel4m/gfp.h>
#include <sel4m/object/tcb.h>
#include <sel4m/object/pid.h>
#include <sel4m/object/untype.h>
#include <sel4m/syscalls.h>

#include <uapi/sel4m/object/tcb.h>

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
	INIT_LIST_HEAD(&tsk->sibling);

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

static pid_t do_fork(pid_t pid, unsigned long ventry, unsigned long varg)
{
	int ret;
	struct task_struct *tsk;

	tsk = tcb_create_task(0);
	if (!tsk)
		return -ENOMEM;

	tsk->pid.pid = pid;
	ret = insert_process_by_pid(tsk);
	if (!ret)
		goto fail_inster_pid;

	snprintf(tsk->comm, TASK_COMM_LEN, "%s-%d", current->comm, pid);
	tsk->stack = kmalloc(THREAD_SIZE, GFP_KERNEL | GFP_ZERO);
	if (!tsk->stack)
		goto fail_stack;

	tcb_set_task_stack_end_magic(tsk);

	return 0;

fail_stack:
	ret = remove_pid_by_process(tsk);
	BUG_ON(!ret);
fail_inster_pid:
	tcb_destroy_task(tsk);

	return -EINVAL;
}

SYSCALL_DEFINE4(tcb_thread, enum tcb_table, table, pid_t, pid,
		unsigned long, fn, unsigned long, arg)
{
	if (!cap_table_test_cap(cap_thread_cap, &current->cap_table))
		return -ENOTCB;

	switch (table) {
		case tcb_create_thread_fn:
			return do_fork(pid, fn, arg);
		case tcb_create_tcb_object:
			printf("tcb_create_tcb_object Nothing TODO!\n");
			return -ECHILD;
		case tcb_clone_task_fn:
			printf("tcb_create_tcb_object Nothing TODO!\n");
			return -ECHILD;
	}

	return -EINVAL;;
}
