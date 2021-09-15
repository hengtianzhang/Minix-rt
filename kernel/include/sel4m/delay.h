
/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SEL4M_DELAY_H_
#define __SEL4M_DELAY_H_

#include <base/common.h>
#include <base/time.h>

#include <asm/delay.h>

#ifndef delay
static inline void delay(u64 cycles)
{
	__delay(cycles);
}
#define delay(x) delay(x)
#endif

#ifndef ndelay
static inline void ndelay(u64 nsecs)
{
	__ndelay(nsecs);
}
#define ndelay(x) ndelay(x)
#endif

#ifndef udelay
static inline void udelay(u64 usecs)
{
	__ndelay(usecs * NSEC_PER_USEC);
}
#define udelay(x) udelay(x)
#endif

#ifndef mdelay
static inline void mdelay(u64 msecs)
{
	__ndelay(msecs * NSEC_PER_MSEC);
}
#define mdelay(x) mdelay(x)
#endif

void msleep(unsigned int msecs);
u64 msleep_interruptible(unsigned int msecs);
void usleep_range(u64 min, u64 max);

static inline void ssleep(unsigned int seconds)
{
	msleep(seconds * 1000);
}

#endif /* !__SEL4M_DELAY_H_ */
