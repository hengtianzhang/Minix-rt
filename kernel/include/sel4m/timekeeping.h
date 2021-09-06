/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_TIMEKEEPING_H_
#define __SEL4M_TIMEKEEPING_H_

#include <base/errno.h>

/* Included from linux/ktime.h */

void timekeeping_init(void);

/*
 * Get and set timeofday
 */
extern int do_settimeofday64(const struct timespec64 *ts);

extern void ktime_get_ts64(struct timespec64 *ts);
extern void ktime_get_real_ts64(struct timespec64 *tv);
extern void ktime_get_coarse_ts64(struct timespec64 *ts);
extern void ktime_get_coarse_real_ts64(struct timespec64 *ts);

/*
 * time64_t base interfaces
 */
extern time64_t ktime_get_seconds(void);
extern time64_t ktime_get_real_seconds(void);

extern ktime_t ktime_get(void);
extern u32 ktime_get_resolution_ns(void);

extern ktime_t ktime_get_real(void);
extern ktime_t ktime_get_coarse_real(void);

extern u64 ktime_get_cycles(void);

static inline u64 ktime_get_ns(void)
{
	return ktime_to_ns(ktime_get());
}

static inline u64 ktime_get_real_ns(void)
{
	return ktime_to_ns(ktime_get_real());
}

extern void read_persistent_clock64(struct timespec64 *ts);
void read_persistent_wall_and_boot_offset(struct timespec64 *wall_clock,
					  struct timespec64 *boot_offset);
extern int update_persistent_clock64(struct timespec64 now);
extern u64 timekeeping_max_deferment(void);

#endif /* !__SEL4M_TIMEKEEPING_H_ */
