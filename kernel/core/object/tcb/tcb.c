#include <sel4m/sched.h>
#include <sel4m/slab.h>
#include <sel4m/gfp.h>
#include <sel4m/object/tcb.h>
#include <sel4m/object/untype.h>
#include <sel4m/syscalls.h>

#include <uapi/sel4m/object/tcb.h>

struct task_struct *tcb_create_task(void)
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

SYSCALL_DEFINE1(tcb_thread, enum tcb_table, table)
{
	if (cap_table_test_cap(cap_thread_cap, &current->cap_table))
		return -ENOTCB;

	return 0;
}
