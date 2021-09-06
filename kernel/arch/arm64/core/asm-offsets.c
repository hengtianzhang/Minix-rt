/*
 * Based on arch/arm/kernel/asm-offsets.c
 *
 * Copyright (C) 1995-2003 Russell King
 *               2001-2002 Keith Owens
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
#include <sel4m/kbuild.h>
#include <sel4m/smp.h>
#include <sel4m/mm_types.h>
#include <sel4m/arm-smccc.h>

int main(void)
{
	DEFINE(CPU_BOOT_STACK,	offsetof(struct secondary_data, stack));
	DEFINE(CPU_BOOT_TASK,		offsetof(struct secondary_data, task));
	BLANK();
	DEFINE(MM_CONTEXT_ID,		offsetof(struct mm_struct, context.id.counter));
	BLANK();
	DEFINE(ARM_SMCCC_RES_X0_OFFS,		offsetof(struct arm_smccc_res, a0));
	DEFINE(ARM_SMCCC_RES_X2_OFFS,		offsetof(struct arm_smccc_res, a2));
	DEFINE(ARM_SMCCC_QUIRK_ID_OFFS,	offsetof(struct arm_smccc_quirk, id));
	DEFINE(ARM_SMCCC_QUIRK_STATE_OFFS,	offsetof(struct arm_smccc_quirk, state));
	BLANK();

	return 0;
}
