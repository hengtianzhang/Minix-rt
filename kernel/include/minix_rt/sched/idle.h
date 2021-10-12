/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MINIX_RT_SCHED_IDLE_H_
#define __MINIX_RT_SCHED_IDLE_H_

/* Attach to any functions which should be considered cpuidle. */
#define __cpuidle	__attribute__((__section__(".cpuidle.text")))

extern struct task_struct idle_threads[CONFIG_NR_CPUS];

extern void early_idle_task_init(void);

extern struct task_struct *service_core_init(int type,
			unsigned long elf_start_archive, const char *name);

#endif /* !__MINIX_RT_SCHED_IDLE_H_ */
