// SPDX-License-Identifier: GPL-2.0
/*
 * This file contains functions which manage high resolution tick
 * related events.
 *
 * Copyright(C) 2005-2006, Thomas Gleixner <tglx@linutronix.de>
 * Copyright(C) 2005-2007, Red Hat, Inc., Ingo Molnar
 * Copyright(C) 2006-2007, Timesys Corp., Thomas Gleixner
 */
// SPDX-License-Identifier: GPL-2.0
/*
 * This file contains functions which manage high resolution tick
 * related events.
 *
 * Copyright(C) 2005-2006, Thomas Gleixner <tglx@linutronix.de>
 * Copyright(C) 2005-2007, Red Hat, Inc., Ingo Molnar
 * Copyright(C) 2006-2007, Timesys Corp., Thomas Gleixner
 */
#include <base/err.h>

#include <minix_rt/cpu.h>
#include <minix_rt/hrtimer.h>
#include <minix_rt/smp.h>

#include "tick-internal.h"

/**
 * tick_program_event
 */
int tick_program_event(ktime_t expires, int force)
{
	struct clock_event_device *dev = tick_cpu_device[smp_processor_id()].evtdev;

	if (unlikely(expires == KTIME_MAX)) {
		/*
		 * We don't need the clock event device any more, stop it.
		 */
		clockevents_switch_state(dev, CLOCK_EVT_STATE_ONESHOT_STOPPED);
		dev->next_event = KTIME_MAX;
		return 0;
	}

	if (unlikely(clockevent_state_oneshot_stopped(dev))) {
		/*
		 * We need the clock event again, configure it in ONESHOT mode
		 * before using it.
		 */
		clockevents_switch_state(dev, CLOCK_EVT_STATE_ONESHOT);
	}

	return clockevents_program_event(dev, expires, force);
}

/**
 * tick_switch_to_oneshot - switch to oneshot mode
 */
int tick_switch_to_oneshot(void (*handler)(struct clock_event_device *))
{
	struct tick_device *td = &tick_cpu_device[smp_processor_id()];
	struct clock_event_device *dev = td->evtdev;

	td->mode = TICKDEV_MODE_ONESHOT;
	dev->event_handler = handler;
	clockevents_switch_state(dev, CLOCK_EVT_STATE_ONESHOT);

	return 0;
}

/**
 * tick_init_highres - switch to high resolution mode
 *
 * Called with interrupts disabled.
 */
int tick_init_highres(void)
{
	return tick_switch_to_oneshot(hrtimer_interrupt);
}

/**
 * tick_check_oneshot_mode - check whether the system is in oneshot mode
 *
 * returns 1 when either nohz or highres are enabled. otherwise 0.
 */
int tick_oneshot_mode_active(void)
{
	u64 flags;
	int ret;

	local_irq_save(flags);
	ret = (tick_cpu_device[smp_processor_id()].mode == TICKDEV_MODE_ONESHOT);
	local_irq_restore(flags);

	return ret;
}
