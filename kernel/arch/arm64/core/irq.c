/*
 * Based on arch/arm/kernel/irq.c
 *
 * Copyright (C) 1992 Linus Torvalds
 * Modifications for ARM processor Copyright (C) 1995-2000 Russell King.
 * Support for Dynamic Tick Timer Copyright (C) 2004-2005 Nokia Corporation.
 * Dynamic Tick Timer written by Tony Lindgren <tony@atomide.com> and
 * Tuukka Tikkanen <tuukka.tikkanen@elektrobit.com>.
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
#include <base/types.h>
#include <base/init.h>

#include <sel4m/irqchip.h>
#include <sel4m/irq.h>
#include <sel4m/page.h>
#include <sel4m/cpumask.h>

u64 *irq_stack_ptr[CONFIG_NR_CPUS] __page_aligned_bss;
u64 irq_stack[CONFIG_NR_CPUS][IRQ_STACK_SIZE/sizeof(s64)] __page_aligned_bss;

static void init_irq_stacks(void)
{
	int cpu;

	for_each_possible_cpu(cpu)
		irq_stack_ptr[cpu] = irq_stack[cpu];
}

void __init init_IRQ(void)
{
	init_irq_stacks();
	irqchip_init();

	if (!handle_arch_irq)
		hang("No interrupt controller found.");
}
