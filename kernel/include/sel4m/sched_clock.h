/*
 * sched_clock.h: support for extending counters to full 64-bit ns counter
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */
#ifndef __MINIX_RT_SCHED_CLOCK_H_
#define __MINIX_RT_SCHED_CLOCK_H_

extern void generic_sched_clock_init(void);

extern void sched_clock_register(u64 (*read)(void), int bits,
				 u64 rate);

int sched_clock_suspend(void);
void sched_clock_resume(void);

#endif /* !__MINIX_RT_SCHED_CLOCK_H_ */
