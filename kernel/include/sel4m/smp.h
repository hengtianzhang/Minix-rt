/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SMP_H_
#define __SEL4M_SMP_H_

#ifndef __ASSEMBLY__

#include <sel4m/types.h>
#include <sel4m/compiler.h>

#include <asm/smp.h>
#include <asm/memory.h>

extern u8 kernel_stack_alloc[CONFIG_NR_CPUS][THREAD_SIZE];

#define smp_processor_id() raw_smp_processor_id()

#endif /* !__ASSEMBLY__ */
#endif /* !__SEL4M_SMP_H_ */
