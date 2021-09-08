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
#define TIF_POLLING_NRFLAG	2
#define TIF_SINGLESTEP		3

#define _TIF_SIGPENDING			(1 << TIF_SIGPENDING)
#define _TIF_NEED_RESCHED		(1 << TIF_NEED_RESCHED)
#define _TIF_POLLING_NRFLAG		(1 << TIF_POLLING_NRFLAG)
#define _TIF_SINGLESTEP			(1 << TIF_SINGLESTEP)

#ifndef __ASSEMBLY__

#include <asm/current.h>

struct task_struct;

/*
 * low level task data that entry.S needs immediate access to.
 */
struct thread_info {
	unsigned long		flags;		/* low level flags */

	union {
		u64		preempt_count;	/* 0 => preemptible, <0 => bug */
		struct {
#ifdef CONFIG_CPU_BIG_ENDIAN
			u32	need_resched;
			u32	count;
#else
			u32	count;
			u32	need_resched;
#endif
		} preempt;
	};
};

#define current_thread_info() 	((struct thread_info *)current)
#define task_thread_info(p)		(&(p->thread_info))

#endif /* !__ASSEMBLY__ */
#endif /* __KERNEL__ */
#endif /* !__ASM_THREAD_INFO_H_ */
