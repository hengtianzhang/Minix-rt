/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SCHED_IDLE_H_
#define __SEL4M_SCHED_IDLE_H_

extern struct task_struct idle_threads[CONFIG_NR_CPUS];

extern void early_idle_task_init(void);

#endif /* !__SEL4M_SCHED_IDLE_H_ */
