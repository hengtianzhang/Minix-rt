/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_STACKPROTECTOR_H_
#define __SEL4M_STACKPROTECTOR_H_

#include <base/compiler.h>

#ifdef CONFIG_STACKPROTECTOR
# include <asm/stackprotector.h>
#else
static inline void boot_init_stack_canary(void)
{
}
#endif

#endif /* !__SEL4M_STACKPROTECTOR_H_ */
