/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SMP_H_
#define __SEL4M_SMP_H_

#ifndef __ASSEMBLY__

#include <base/compiler.h>

#include <base/types.h>

#include <asm/smp.h>
#include <asm/memory.h>

extern u8 kernel_stack_alloc[CONFIG_NR_CPUS][THREAD_SIZE];

#define smp_processor_id() raw_smp_processor_id()

void smp_setup_processor_id(void);

extern int __boot_cpu_id;

static inline int get_boot_cpu_id(void)
{
	return __boot_cpu_id;
}

/*
 * sends a 'reschedule' event to another CPU:
 */
extern void smp_send_reschedule(int cpu);

/*
 * Mark the boot cpu "online" so that it can call console drivers in
 * printk() and can access its per-cpu storage.
 */
void smp_prepare_boot_cpu(void);

#endif /* !__ASSEMBLY__ */
#endif /* !__SEL4M_SMP_H_ */
