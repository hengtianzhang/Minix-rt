/*
 * Based on arch/arm/kernel/time.c
 *
 * Copyright (C) 1991, 1992, 1995  Linus Torvalds
 * Modifications for ARM (C) 1994-2001 Russell King
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
#include <base/common.h>
#include <base/time.h>
#include <base/init.h>

#include <minix_rt/clockchips.h>

#include <clocksource/arm_arch_timer.h>

void __init time_init(void)
{
	u32 arch_timer_rate;

	/* TODO now nothing todo! */
	//of_clk_init(NULL);
	timer_probe();

	arch_timer_rate = arch_timer_get_rate();
	if (!arch_timer_rate)
		hang("Unable to initialise architected timer.\n");
}
