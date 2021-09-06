// SPDX-License-Identifier: GPL-2.0
/*
 * This file contains the base functions to manage periodic tick
 * related events.
 *
 * Copyright(C) 2005-2006, Thomas Gleixner <tglx@linutronix.de>
 * Copyright(C) 2005-2007, Red Hat, Inc., Ingo Molnar
 * Copyright(C) 2006-2007, Timesys Corp., Thomas Gleixner
 */

#include <base/init.h>

#include <sel4m/clockchips.h>
#include <sel4m/smp.h>

#include "tick-internal.h"
#include "tick-sched.h"

/*
 * tick_do_timer_cpu is a timer core internal variable which holds the CPU NR
 * which is responsible for calling do_timer(), i.e. the timekeeping stuff. This
 * variable has two functions:
 *
 * 1) Prevent a thundering herd issue of a gazillion of CPUs trying to grab the
 *    timekeeping lock all at once. Only the CPU which is assigned to do the
 *    update is handling it.
 *
 * 2) Hand off the duty in the NOHZ idle case by setting the value to
 *    TICK_DO_TIMER_NONE, i.e. a non existing CPU. So the next cpu which looks
 *    at it will take over and keep the time keeping alive.  The handover
 *    procedure also covers cpu hotplug.
 */
int tick_do_timer_cpu __read_mostly = TICK_DO_TIMER_BOOT;

struct tick_device tick_cpu_device[CONFIG_NR_CPUS];

/*
 * Check, if the new registered device should be used. Called with
 * clockevents_lock held and interrupts disabled.
 */
void tick_check_new_device(struct clock_event_device *newdev)
{
	struct tick_device *td;
	int cpu;

	cpu = smp_processor_id();

	td = &tick_cpu_device[cpu];

	if (!td->evtdev) {
		if (tick_do_timer_cpu == TICK_DO_TIMER_BOOT)
			tick_do_timer_cpu = cpu;
	}

	td->evtdev = newdev;
	td->mode = TICKDEV_MODE_ONESHOT;
}
