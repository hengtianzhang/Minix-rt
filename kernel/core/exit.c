/*
 *  linux/kernel/exit.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */
#include <base/compiler.h>

#include <minix_rt/sched.h>
#include <minix_rt/object/notifier.h>
#include <minix_rt/stat.h>

#include <asm/current.h>

extern void tcb_do_exit(struct task_struct *tsk, int flags);
void __noreturn do_exit(long code)
{
	notifier_table_set_notifier(code, &current->parent->notifier.notifier_table);
	tcb_do_exit(current, -1);
	BUG(); /* here BUG */
}
