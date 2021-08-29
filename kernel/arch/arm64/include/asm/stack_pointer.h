/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_STACK_POINTER_H_
#define __ASM_STACK_POINTER_H_

#include <sel4m/types.h>

/*
 * how to get the current stack pointer from C
 */
register u64 current_stack_pointer asm ("sp");

#endif /* !__ASM_STACK_POINTER_H_ */
