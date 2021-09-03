/*
 * Copyright (C) 2017 ARM Ltd.
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
#ifndef __ASM_DAIFFLAGS_H_
#define __ASM_DAIFFLAGS_H_

#include <sel4m/irqflags.h>

#define DAIF_PROCCTX		0
#define DAIF_PROCCTX_NOIRQ	PSR_I_BIT

/* mask/save/unmask/restore all exceptions, including interrupts. */
static inline void local_daif_mask(void)
{
	asm volatile(
		"msr	daifset, #0xf		// local_daif_mask\n"
		:
		:
		: "memory");
}

static inline u64 local_daif_save(void)
{
	u64 flags;

	flags = arch_local_save_flags();

	local_daif_mask();

	return flags;
}

static inline void local_daif_unmask(void)
{
	asm volatile(
		"msr	daifclr, #0xf		// local_daif_unmask"
		:
		:
		: "memory");
}

static inline void local_daif_restore(u64 flags)
{
	arch_local_irq_restore(flags);
}

#endif /* !__ASM_DAIFFLAGS_H_ */
