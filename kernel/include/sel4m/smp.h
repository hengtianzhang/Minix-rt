/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SMP_H_
#define __SEL4M_SMP_H_

#include <asm/smp.h>

#ifndef __ASSEMBLY__

#include <sel4m/types.h>
#include <sel4m/compiler.h>

#include <asm/memory.h>

extern u8 kernel_stack_alloc[CONFIG_NR_CPUS][THREAD_SIZE];

#endif /* !__ASSEMBLY__ */
#endif /* !__SEL4M_SMP_H_ */
