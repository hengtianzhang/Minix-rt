/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_SCHED_SCHED_CLOCK_H_
#define __SEL4M_SCHED_SCHED_CLOCK_H_

#include <sel4m/sched_clock.h>
#include <sel4m/smp.h>

extern u64 notrace sched_clock(void);

extern void sched_clock_init(void);

#endif /* !__SEL4M_SCHED_SCHED_CLOCK_H_ */
