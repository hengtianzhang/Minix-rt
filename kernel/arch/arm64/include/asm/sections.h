/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Copyright (C) 2016 ARM Limited
 */
#ifndef __ASM_SECTIONS_H_
#define __ASM_SECTIONS_H_

#include <asm-generic/sections.h>

extern char __alt_instructions[], __alt_instructions_end[];
extern char __idmap_text_start[], __idmap_text_end[];
extern char __initdata_begin[], __initdata_end[];
extern char __inittext_begin[], __inittext_end[];
extern char __irqentry_text_start[], __irqentry_text_end[];
extern char __mmuoff_data_start[], __mmuoff_data_end[];

#endif /* !__ASM_SECTIONS_H_ */
