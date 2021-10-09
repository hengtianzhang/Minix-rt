/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MINIX_RT_SCHED_SCHED_CLOCK_H_
#define __MINIX_RT_SCHED_SCHED_CLOCK_H_

#include <minix_rt/sched_clock.h>
#include <minix_rt/smp.h>

extern u64 notrace sched_clock(void);

extern void sched_clock_init(void);

static inline u64 local_clock(void)
{
	return sched_clock();
}

#endif /* !__MINIX_RT_SCHED_SCHED_CLOCK_H_ */
