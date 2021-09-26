/*
 *  linux/kernel/exit.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */
#include <base/compiler.h>

#include <sel4m/sched.h>
#include <sel4m/object/tcb.h>
#include <sel4m/object/notifier.h>
#include <sel4m/stat.h>

#include <asm/current.h>

void __noreturn do_exit(long code)
{
	notifier_table_set_notifier(code, &current->parent->notifier.notifier_table);
	tcb_do_exit(current, -1);
	BUG(); /* here BUG */
}
