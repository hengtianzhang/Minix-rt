/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __ASM_GENERIC_SECTIONS_H_
#define __ASM_GENERIC_SECTIONS_H_

/* References to section boundaries */

#include <base/compiler.h>
#include <base/types.h>

extern char _text[], _stext[], _etext[];
extern char _data[], _sdata[], _edata[];
extern char __bss_start[], __bss_stop[];
extern char __init_begin[], __init_end[];
extern char _sinittext[], _einittext[];
extern char __start_ro_after_init[], __end_ro_after_init[];
extern char _end[];
extern char __entry_text_start[], __entry_text_end[];
extern char __start_rodata[], __end_rodata[];
extern char __irqentry_text_start[], __irqentry_text_end[];
extern char __start_once[], __end_once[];

/**
 * is_kernel_rodata - checks if the pointer address is located in the
 *                    .rodata section
 *
 * @addr: address to check
 *
 * Returns: true if the address is located in .rodata, false otherwise.
 */
static inline bool is_kernel_rodata(u64 addr)
{
	return addr >= (u64)__start_rodata &&
	       addr < (u64)__end_rodata;
}

#endif /* !__ASM_GENERIC_SECTIONS_H_ */
