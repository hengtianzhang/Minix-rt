/*
 *  linux/kernel/exit.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 */
#include <base/compiler.h>

#include <sel4m/stat.h>

void __noreturn do_exit(long code)
{
	while (1);
}
