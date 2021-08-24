/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_MISC_ATOMIC_H_
#define __ASM_GENERIC_MISC_ATOMIC_H_

#include <misc/types.h>

typedef struct {
	s32 counter;
} atomic_t;

#ifdef CONFIG_64BIT
typedef struct {
	s64 counter;
} atomic64_t;
#endif

#endif /* !__ASM_GENERIC_MISC_ATOMIC_H_ */
