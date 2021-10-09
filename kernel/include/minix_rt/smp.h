/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MINIX_RT_SMP_H_
#define __MINIX_RT_SMP_H_

#ifndef __ASSEMBLY__

#include <base/compiler.h>
#include <base/types.h>
#include <base/llist.h>

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

void smp_prepare_cpus(unsigned int max_cpus);

/*
 * Bring a CPU up
 */
extern int __cpu_up(unsigned int cpunum, struct task_struct *tidle);

/*
 * Final polishing of CPUs
 */
extern void smp_cpus_done(unsigned int max_cpus);

/*
 * stops all CPUs but the current one:
 */
extern void smp_send_stop(void);

extern void smp_init(void);

/*
 * Call a function on all processors
 */
extern int on_each_cpu(void (*func) (void *info), void *info, int wait);

typedef void (*smp_call_func_t)(void *info);
struct call_single_data {
	union {
		struct list_head list;
		struct llist_node llist;
	};
	smp_call_func_t func;
	void *info;
	u16 flags;
};

#define get_cpu()		({ preempt_disable(); smp_processor_id(); })
#define put_cpu()		preempt_enable()

void call_function_init(void);
void generic_smp_call_function_single_interrupt(void);
#define generic_smp_call_function_interrupt \
	generic_smp_call_function_single_interrupt

/*
 * Call a function on all other processors
 */
int smp_call_function(smp_call_func_t func, void *info, int wait);
void smp_call_function_many(const struct cpumask *mask,
			    smp_call_func_t func, void *info, bool wait);

int smp_call_function_single(int cpu, smp_call_func_t func, void *info,
			     int wait);

#endif /* !__ASSEMBLY__ */
#endif /* !__MINIX_RT_SMP_H_ */
