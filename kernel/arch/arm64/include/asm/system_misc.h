/*
 * Based on arch/arm/include/asm/system_misc.h
 *
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
#ifndef __ASM_SYSTEM_MISC_H_
#define __ASM_SYSTEM_MISC_H_

#ifndef __ASSEMBLY__

struct pt_regs;
struct task_struct;

void die(const char *msg, struct pt_regs *regs, int err);
void show_pte(unsigned long addr);
extern void __show_regs(struct pt_regs *);

/*
 * TASK is a pointer to the task whose backtrace we want to see (or NULL for current
 * task), SP is the stack pointer of the first frame that should be shown in the back
 * trace (or NULL if the entire call-chain of the task should be shown).
 */
extern void show_stack(struct task_struct *task, unsigned long *sp);

void arm64_notify_die(const char *str, struct pt_regs *regs,
		      int signo, int sicode, void __user *addr,
		      int err);
#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_SYSTEM_MISC_H_ */
