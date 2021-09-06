/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __KERNEL_TIME_TIMEKEEPING_H_
#define __KERNEL_TIME_TIMEKEEPING_H_

#include <sel4m/seqlock.h>
#include <sel4m/clocksource.h>

/*
 * Internal interfaces for kernel/time/
 */
extern ktime_t ktime_get_update_offsets_now(unsigned int *cwsseq, ktime_t *offs_real);

extern int timekeeping_valid_for_hres(void);
extern void timekeeping_warp_clock(void);
extern int timekeeping_suspend(void);
extern void timekeeping_resume(void);

extern void do_timer(u64 ticks);
extern void update_wall_time(void);

extern seqlock_t jiffies_lock;

extern int timekeeping_notify(struct clocksource *clock);

#define CS_NAME_LEN	32

#endif /* !__KERNEL_TIME_TIMEKEEPING_H_ */
