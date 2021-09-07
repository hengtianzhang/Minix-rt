/*
 * Mutexes: blocking mutual exclusion locks
 *
 * started by Ingo Molnar:
 *
 *  Copyright (C) 2004, 2005, 2006 Red Hat, Inc., Ingo Molnar <mingo@redhat.com>
 *
 * This file contains the main data structure and API definitions.
 */
#ifndef __SEL4M_MUTEX_H_
#define __SEL4M_MUTEX_H_

#include <base/atomic.h>

struct mutex {
	/* 1: unlocked, 0: locked, negative: locked, possible waiters */
	atomic_t		count;
};

#define DEFINE_MUTEX(mutexname) \
    struct mutex mutexname = { .count = ATOMIC_INIT(1), }

#endif /* !__SEL4M_MUTEX_H_ */
