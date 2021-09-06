/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_TIME64_H_
#define __BASE_TIME64_H_

#include <base/math64.h>

typedef __s64 time64_t;
typedef __u64 timeu64_t;

/* CONFIG_64BIT_TIME enables new 64 bit time_t syscalls in the compat path
 * and 32-bit emulation.
 */
#ifndef CONFIG_64BIT_TIME
#define __common_timespec timespec
#define __common_itimerspec itimerspec
#endif

#ifndef _STRUCT_TIMESPEC
#define _STRUCT_TIMESPEC
struct timespec {
	s64		tv_sec;			/* seconds */
	s64		tv_nsec;		/* nanoseconds */
};
#endif

struct timeval {
	s64		tv_sec;		/* seconds */
	s64		tv_usec;	/* microseconds */
};

struct timezone {
	int	tz_minuteswest;	/* minutes west of Greenwich */
	int	tz_dsttime;	/* type of dst correction */
};
 
/*
 * Names of the interval timers, and structure
 * defining a timer setting:
 */
#define	ITIMER_REAL		0
#define	ITIMER_VIRTUAL		1
#define	ITIMER_PROF		2

struct itimerspec {
	struct timespec it_interval;	/* timer period */
	struct timespec it_value;	/* timer expiration */
};

struct itimerval {
	struct timeval it_interval;	/* timer interval */
	struct timeval it_value;	/* current value */
};

#ifndef __common_timespec
struct __common_timespec {
	s64       		  tv_sec;                 /* seconds */
	s64               tv_nsec;                /* nanoseconds */
};
#endif

#ifndef __common_itimerspec
struct __common_itimerspec {
	struct __common_timespec it_interval;    /* timer period */
	struct __common_timespec it_value;       /* timer expiration */
};
#endif

/*
 * legacy timeval structure, only embedded in structures that
 * traditionally used 'timeval' to pass time intervals (not absolute
 * times). Do not add new users. If user space fails to compile
 * here, this is probably because it is not y2038 safe and needs to
 * be changed to use another interface.
 */
struct __common_old_timeval {
	s64 	tv_sec;
	s64 	tv_usec;
};

/*
 * The IDs of the various system clocks (for POSIX.1b interval timers):
 */
#define CLOCK_REALTIME			0
#define CLOCK_MONOTONIC			1

#define MAX_CLOCKS			2
#define CLOCKS_MASK			(CLOCK_REALTIME | CLOCK_MONOTONIC)
#define CLOCKS_MONO			CLOCK_MONOTONIC

/*
 * The various flags for setting POSIX.1b interval timers:
 */
#define TIMER_ABSTIME			0x01

struct timespec64 {
	time64_t	tv_sec;			/* seconds */
	s64		tv_nsec;		/* nanoseconds */
};

struct itimerspec64 {
	struct timespec64 it_interval;
	struct timespec64 it_value;
};

/* Parameters used to convert the timespec values: */
#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

/* Located here for timespec[64]_valid_strict */
#define TIME64_MAX			((s64)~((u64)1 << 63))
#define KTIME_MAX			((s64)~((u64)1 << 63))
#define KTIME_SEC_MAX			(KTIME_MAX / NSEC_PER_SEC)

static inline int timespec64_equal(const struct timespec64 *a,
				   const struct timespec64 *b)
{
	return (a->tv_sec == b->tv_sec) && (a->tv_nsec == b->tv_nsec);
}

/*
 * lhs < rhs:  return <0
 * lhs == rhs: return 0
 * lhs > rhs:  return >0
 */
static inline int timespec64_compare(const struct timespec64 *lhs, const struct timespec64 *rhs)
{
	if (lhs->tv_sec < rhs->tv_sec)
		return -1;
	if (lhs->tv_sec > rhs->tv_sec)
		return 1;
	return lhs->tv_nsec - rhs->tv_nsec;
}

extern void set_normalized_timespec64(struct timespec64 *ts, time64_t sec, s64 nsec);

static inline struct timespec64 timespec64_add(struct timespec64 lhs,
						struct timespec64 rhs)
{
	struct timespec64 ts_delta;
	set_normalized_timespec64(&ts_delta, lhs.tv_sec + rhs.tv_sec,
				lhs.tv_nsec + rhs.tv_nsec);
	return ts_delta;
}

/*
 * sub = lhs - rhs, in normalized form
 */
static inline struct timespec64 timespec64_sub(struct timespec64 lhs,
						struct timespec64 rhs)
{
	struct timespec64 ts_delta;
	set_normalized_timespec64(&ts_delta, lhs.tv_sec - rhs.tv_sec,
				lhs.tv_nsec - rhs.tv_nsec);
	return ts_delta;
}

/*
 * Returns true if the timespec64 is norm, false if denorm:
 */
static inline bool timespec64_valid(const struct timespec64 *ts)
{
	/* Dates before 1970 are bogus */
	if (ts->tv_sec < 0)
		return false;
	/* Can't have more nanoseconds then a second */
	if ((u64)ts->tv_nsec >= NSEC_PER_SEC)
		return false;
	return true;
}

static inline bool timespec64_valid_strict(const struct timespec64 *ts)
{
	if (!timespec64_valid(ts))
		return false;
	/* Disallow values that could overflow ktime_t */
	if ((u64)ts->tv_sec >= KTIME_SEC_MAX)
		return false;
	return true;
}

/**
 * timespec64_to_ns - Convert timespec64 to nanoseconds
 * @ts:		pointer to the timespec64 variable to be converted
 *
 * Returns the scalar nanosecond representation of the timespec64
 * parameter.
 */
static inline s64 timespec64_to_ns(const struct timespec64 *ts)
{
	return ((s64) ts->tv_sec * NSEC_PER_SEC) + ts->tv_nsec;
}

/**
 * ns_to_timespec64 - Convert nanoseconds to timespec64
 * @nsec:	the nanoseconds value to be converted
 *
 * Returns the timespec64 representation of the nsec parameter.
 */
extern struct timespec64 ns_to_timespec64(const s64 nsec);

/**
 * timespec64_add_ns - Adds nanoseconds to a timespec64
 * @a:		pointer to timespec64 to be incremented
 * @ns:		unsigned nanoseconds value to be added
 *
 * This must always be inlined because its used from the x86-64 vdso,
 * which cannot call other kernel functions.
 */
static __always_inline void timespec64_add_ns(struct timespec64 *a, u64 ns)
{
	a->tv_sec += __iter_div_u64_rem(a->tv_nsec + ns, NSEC_PER_SEC, &ns);
	a->tv_nsec = ns;
}

/*
 * timespec64_add_safe assumes both values are positive and checks for
 * overflow. It will return TIME64_MAX in case of overflow.
 */
extern struct timespec64 timespec64_add_safe(const struct timespec64 lhs,
					 const struct timespec64 rhs);

#endif /* !__BASE_TIME64_H_ */
