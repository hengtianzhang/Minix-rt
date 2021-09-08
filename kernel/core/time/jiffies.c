// SPDX-License-Identifier: GPL-2.0
/*
 *  Kernel internal timers
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  1997-01-28  Modified by Finn Arne Gangstad to make timers scale better.
 *
 *  1997-09-10  Updated NTP code according to technical memorandum Jan '96
 *              "A Kernel Model for Precision Timekeeping" by Dave Mills
 *  1998-12-24  Fixed a xtime SMP race (we need the xtime_lock rw spinlock to
 *              serialize accesses to xtime/lost_ticks).
 *                              Copyright (C) 1998  Andrea Arcangeli
 *  1999-03-10  Improved NTP compatibility by Ulrich Windl
 *  2002-05-31	Move sys_sysinfo here and make its locking sane, Robert Love
 *  2000-10-05  Implemented scalable SMP per-CPU timer handling.
 *                              Copyright (C) 2000, 2001, 2002  Ingo Molnar
 *              Designed by David S. Miller, Alexey Kuznetsov and Ingo Molnar
 */
#include <base/time.h>
#include <base/cache.h>

#include <sel4m/ktime.h>
#include <sel4m/jiffies.h>
#include <sel4m/hrtimer.h>
#include <sel4m/sched.h>
#include <sel4m/seqlock.h>
#include <sel4m/smp.h>

#include "timekeeping.h"
#include "tick-internal.h"

u64 __cacheline_aligned_in_smp jiffies_64 = 0;

__cacheline_aligned_in_smp DEFINE_SEQLOCK(jiffies_lock);

u64 get_jiffies_64(void)
{
	u64 seq;
	u64 ret;

	do {
		seq = read_seqbegin(&jiffies_lock);
		ret = jiffies_64;
	} while (read_seqretry(&jiffies_lock, seq));
	return ret;
}

static struct hrtimer jiffies_timer[CONFIG_NR_CPUS];

static enum hrtimer_restart tick_periodic(struct hrtimer *hrt)
{
	if (tick_do_timer_cpu == smp_processor_id()) {
		write_seqlock(&jiffies_lock);
		jiffies_64++;
		write_sequnlock(&jiffies_lock);

		update_wall_time();
	}

	scheduler_tick();

	hrtimer_forward_now(hrt, jiffies_to_nsecs(1));

	return HRTIMER_RESTART;
}

void __init system_tick_init(void)
{
	u64 flags;
	struct hrtimer *timer = &jiffies_timer[smp_processor_id()];

	local_irq_save(flags);
	hrtimer_init(timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	timer->function = tick_periodic;
	hrtimer_start(timer, jiffies_to_nsecs(1), HRTIMER_MODE_REL);
	local_irq_restore(flags);
}
