/*
 * Based on arch/arm/include/asm/thread_info.h
 *
 * Copyright (C) 2002 Russell King.
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_THREAD_INFO_H_
#define __ASM_THREAD_INFO_H_

#ifdef __KERNEL__

#include <base/compiler.h>

#define TIF_SIGPENDING		0
#define TIF_NEED_RESCHED	1
#define TIF_FSCHECK			2	/* Check FS is USER_DS on return */
#define TIF_SINGLESTEP		3
#define TIF_POLLING_NRFLAG	4

#define _TIF_SIGPENDING			(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED		(1 << TIF_NEED_RESCHED)
#define _TIF_POLLING_NRFLAG		(1 << TIF_POLLING_NRFLAG)
#define _TIF_SINGLESTEP			(1 << TIF_SINGLESTEP)
#define _TIF_FSCHECK			(1 << TIF_FSCHECK)

#define _TIF_WORK_MASK		(_TIF_SIGPENDING | _TIF_NEED_RESCHED | _TIF_FSCHECK)

#define _TIF_SYSCALL_WORK	(0)

#ifndef __ASSEMBLY__

#include <asm/current.h>
#include <asm/processor.h>

typedef unsigned long mm_segment_t;

struct task_struct;

/*
 * low level task data that entry.S needs immediate access to.
 */
struct thread_info {
	unsigned long		flags;		/* low level flags */
	mm_segment_t		addr_limit;	/* address limit */
	u64		preempt_count;	/* 0 => preemptible, <0 => bug */
};

#define thread_saved_pc(tsk)	\
	((u64)(tsk->thread.cpu_context.pc))
#define thread_saved_sp(tsk)	\
	((u64)(tsk->thread.cpu_context.sp))
#define thread_saved_fp(tsk)	\
	((u64)(tsk->thread.cpu_context.fp))

#define current_thread_info() 	((struct thread_info *)current)
#define task_thread_info(p)		(&(p->thread_info))

#define INIT_THREAD_INFO(tsk)				\
{								\
	.flags		= _TIF_POLLING_NRFLAG,		\
	.preempt_count	= 0,		\
	.addr_limit	= KERNEL_DS,				\
}

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */
#endif /* !__ASM_THREAD_INFO_H_ */
