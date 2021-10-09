/* SPDX-License-Identifier: GPL-2.0 */
/*
 * GCC stack protector support.
 *
 * Stack protector works by putting predefined pattern at the start of
 * the stack frame and verifying that it hasn't been overwritten when
 * returning from the function.  The pattern is called stack canary
 * and gcc expects it to be defined by a global variable called
 * "__stack_chk_guard" on ARM.  This unfortunately means that on SMP
 * we cannot have a different canary value per task.
 */

#ifndef __ASM_STACKPROTECTOR_H_
#define __ASM_STACKPROTECTOR_H_

#include <base/compiler.h>
#include <base/types.h>
#include <base/random.h>

#include <generated/version.h>

#include <asm/current.h>

extern unsigned long long __stack_chk_guard;

/*
 * Initialize the stackprotector canary value.
 *
 * NOTE: this must only be called from functions that never return,
 * and it must always be inlined.
 */
static __always_inline void boot_init_stack_canary(struct task_struct *tsk)
{
	u64 canary = random();

	canary ^= MINIX_RT_VERSION_CODE;
	canary &= CANARY_MASK;

	tsk->stack_canary = canary;

	__stack_chk_guard = tsk->stack_canary;
}

#endif	/* !__ASM_STACKPROTECTOR_H_ */
