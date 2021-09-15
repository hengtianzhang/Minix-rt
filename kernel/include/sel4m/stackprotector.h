/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_STACKPROTECTOR_H_
#define __SEL4M_STACKPROTECTOR_H_

#include <base/compiler.h>

#ifdef CONFIG_STACKPROTECTOR
# include <asm/stackprotector.h>
#else
struct task_struct;
static inline void boot_init_stack_canary(struct task_struct *tsk)
{
}
#endif

#endif /* !__SEL4M_STACKPROTECTOR_H_ */
