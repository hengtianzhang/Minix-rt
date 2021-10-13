/*
 *  linux/kernel/exit.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */
#include <base/compiler.h>

#include <minix_rt/sched.h>
#include <minix_rt/notifier.h>
#include <minix_rt/stat.h>

#include <asm/current.h>

void task_do_exit(struct task_struct *tsk, int flags)
{
	tsk->exit_code = flags;
	tsk->exit_signal = SIGCHLD;
	tsk->exit_state = EXIT_ZOMBIE;
	tsk->flags |= PF_EXITING;
	list_del(&tsk->children_list);
	list_add(&tsk->children_list, &tsk->parent->children_exit);

	put_task_struct(tsk);
	preempt_disable();
	tsk->state = TASK_DEAD;
	set_tsk_thread_flag(tsk->parent, TIF_SIGPENDING);
	schedule();
	BUG();
	/* Avoid "noreturn function does return".  */
	for (;;)
		cpu_relax();	/* For when BUG is null */
}

void __noreturn do_exit(long code)
{
	notifier_table_set_notifier(code, &current->parent->notifier.notifier_table);
	task_do_exit(current, -1);
	BUG(); /* here BUG */
}
