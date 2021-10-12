/*
 *  linux/kernel/fork.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */

/*
 *  'fork.c' contains the help-routines for the 'fork' system call
 * (see also entry.S and others).
 * Fork is rather simple, once you get the hang of it, but the memory
 * management can be a bitch. See 'mm/memory.c': 'copy_page_range()'
 */
#include <minix_rt/sched.h>
#include <minix_rt/slab.h>
#include <minix_rt/gfp.h>
#include <minix_rt/pid.h>
#include <minix_rt/mmap.h>
#include <minix_rt/syscalls.h>

#include <uapi/minix_rt/magic.h>

asmlinkage void ret_from_fork(void) asm("ret_from_fork");

struct task_struct *task_create_tsk(unsigned int flags)
{
	struct task_struct *tsk;
	struct mm_struct *mm;

	tsk = kmalloc(sizeof (*tsk), GFP_KERNEL | GFP_ZERO);
	if (!tsk)
		return NULL;

	mm = mmap_alloc_mm_struct();
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

	return tsk;

fail_mm:
	kfree(tsk);

	return NULL;
}

void task_destroy_tsk(struct task_struct *tsk)
{
	if (!tsk)
		return;

	mmap_free_mm_struct(tsk->mm);
	kfree(tsk);
}

void task_set_stack_end_magic(struct task_struct *tsk)
{
	unsigned long *stackend;

	stackend = end_of_stack(tsk);
	*stackend = STACK_END_MAGIC;	/* for overflow detection */
}

pid_t do_fork(unsigned long ventry, unsigned long varg,
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

	tsk = task_create_tsk(clone_flags);
	if (!tsk)
		return -ENOMEM;

	ret = pid_alloc_pid(tsk);
	if (ret)
		goto fail_inster_pid;

	snprintf(tsk->comm, TASK_COMM_LEN, "%s-%d", current->comm, tsk->pid.pid);
	tsk->stack = kmalloc(THREAD_SIZE, GFP_KERNEL | GFP_ZERO);
	if (!tsk->stack)
		goto fail_stack;

	task_set_stack_end_magic(tsk);

	ret = mmap_copy_mm(tsk, current, &stack_top, &ipcptr);
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
	task_destroy_tsk(tsk);
fail_ventry:

	return -EINVAL;
}
