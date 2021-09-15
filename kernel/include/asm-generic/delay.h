/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_DELAY_H
#define __ASM_GENERIC_DELAY_H

extern void __delay(u64 cycles);
extern void __ndelay(u64 nsecs);

#endif /* __ASM_GENERIC_DELAY_H */
