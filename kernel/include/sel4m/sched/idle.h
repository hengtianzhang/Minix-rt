/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SCHED_IDLE_H_
#define __SEL4M_SCHED_IDLE_H_

extern struct task_struct idle_threads[CONFIG_NR_CPUS];
extern u8 idle_stack[CONFIG_NR_CPUS][THREAD_SIZE];

void idle_prepare_init(struct task_struct *idle, int cpu);

#endif /* !__SEL4M_SCHED_IDLE_H_ */
