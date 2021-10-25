/*
 * Based on arch/arm/include/asm/traps.h
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
#ifndef __ASM_TRAP_H_
#define __ASM_TRAP_H_

#include <base/compiler.h>
#include <base/list.h>

struct pt_regs;

struct undef_hook {
	struct list_head node;
	u32 instr_mask;
	u32 instr_val;
	u64 pstate_mask;
	u64 pstate_val;
	int (*fn)(struct pt_regs *regs, u32 instr);
};

void register_undef_hook(struct undef_hook *hook);
void unregister_undef_hook(struct undef_hook *hook);
void arm64_force_sig_fault(int signo, int code, void __user *addr, const char *str);
void arm64_force_sig_mceerr(int code, void __user *addr, short lsb, const char *str);
void arm64_skip_faulting_instruction(struct pt_regs *regs, unsigned long size);
void arm64_notify_segfault(unsigned long addr);

#endif /* !__ASM_TRAP_H_ */
