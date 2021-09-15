/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SCHED_IDLE_H_
#define __SEL4M_SCHED_IDLE_H_

/* Attach to any functions which should be considered cpuidle. */
#define __cpuidle	__attribute__((__section__(".cpuidle.text")))

extern struct task_struct idle_threads[CONFIG_NR_CPUS];

extern void early_idle_task_init(void);

extern void service_core_init(void);

#endif /* !__SEL4M_SCHED_IDLE_H_ */
