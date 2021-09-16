/*
 * Based on arch/arm/include/asm/processor.h
 *
 * Copyright (C) 1995-1999 Russell King
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
#ifndef __ASM_PROCESSOR_H_
#define __ASM_PROCESSOR_H_

#ifndef __ASSEMBLY__
#ifdef __KERNEL__

#include <base/string.h>

#include <asm/memory.h>
#include <asm/ptrace.h>

#include <asm/base/processor.h>

#define KERNEL_DS		UL(-1)
#define USER_DS			((UL(1) << MAX_USER_VA_BITS) - 1)

#define DEFAULT_MAP_WINDOW_64	(UL(1) << VA_BITS)
#define TASK_SIZE_64		(UL(1) << vabits_user)

#define DEFAULT_MAP_WINDOW	DEFAULT_MAP_WINDOW_64
#define TASK_SIZE		TASK_SIZE_64

#define STACK_TOP_MAX		DEFAULT_MAP_WINDOW_64
#define TASK_UNMAPPED_BASE	(PAGE_ALIGN(DEFAULT_MAP_WINDOW / 4))

#define STACK_TOP		STACK_TOP_MAX

struct cpu_context {
	unsigned long x19;
	unsigned long x20;
	unsigned long x21;
	unsigned long x22;
	unsigned long x23;
	unsigned long x24;
	unsigned long x25;
	unsigned long x26;
	unsigned long x27;
	unsigned long x28;
	unsigned long fp;
	unsigned long sp;
	unsigned long pc;
};

struct thread_struct {
	struct cpu_context	cpu_context;	/* cpu context */

	struct {
		unsigned long	tp_value;	/* TLS register */
	} uw;

	unsigned long		fault_address;	/* fault info */
	unsigned long		fault_code;	/* ESR_EL1 value */
};

extern struct task_struct *cpu_switch_to(struct task_struct *prev,
					 struct task_struct *next);

#define task_user_tls(t)	(&(t)->thread.uw.tp_value)

#define task_pt_regs(p) \
	((struct pt_regs *)(THREAD_SIZE + task_stack_page(p)) - 1)

void tls_preserve_current_state(void);

static inline void start_thread_common(struct pt_regs *regs, unsigned long pc)
{
	memset(regs, 0, sizeof(*regs));
	forget_syscall(regs);
	regs->pc = pc;
}

static inline void start_thread(struct pt_regs *regs, unsigned long pc,
				unsigned long sp)
{
	start_thread_common(regs, pc);
	regs->pstate = PSR_MODE_EL0t;
	regs->pstate |= PSR_SSBS_BIT;
	regs->sp = sp;
}

#endif /* __KERNEL__ */
#endif /* !__ASSEMBLY__*/
#endif /* !__ASM_PROCESSOR_H_ */
