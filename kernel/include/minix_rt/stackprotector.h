/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MINIX_RT_STACKPROTECTOR_H_
#define __MINIX_RT_STACKPROTECTOR_H_

#include <base/compiler.h>

#ifdef CONFIG_STACKPROTECTOR
# include <asm/stackprotector.h>
#else
struct task_struct;
static inline void boot_init_stack_canary(struct task_struct *tsk)
{
}
#endif

#endif /* !__MINIX_RT_STACKPROTECTOR_H_ */
