/* SPDX-License-Identifier: GPL-2.0 */
/*
 * ld script to make ARM Linux kernel
 * taken from the i386 version by Russell King
 * Written by Martin Mares <mj@atrey.karlin.mff.cuni.cz>
 */

#include <linux/stddef.h>

#define ARM_EXIT_KEEP(x)	x
#define ARM_EXIT_DISCARD(x)

#ifndef BSS_FIRST_SECTIONS
#define BSS_FIRST_SECTIONS
#endif

/* Section used for early init (in .S files) */
#define HEAD_TEXT  KEEP(*(.head.text))

#define BSS_MAIN .bss .bss.[0-9a-zA-Z_]*

#define NOTES								\
	.notes : AT(ADDR(.notes) - 0) {			\
		__start_notes = .;					\
		KEEP(*(.note.*))					\
		__stop_notes = .;					\
	}

SECTIONS
{
	/*
	 * XXX: The linker does not define how output sections are
	 * assigned to input sections when there are multiple statements
	 * matching the same input section name.  There is no documented
	 * order of matching.
	 */
	/DISCARD/ : {
		ARM_EXIT_DISCARD(EXIT_TEXT)
		ARM_EXIT_DISCARD(EXIT_DATA)
		*(.discard)
		*(.discard.*)
		*(.interp .dynamic)
		*(.dynsym .dynstr .hash .gnu.hash)
		*(.eh_frame)
	}

	. = 0x0;

	.head.text : {
		_text = .;
		HEAD_TEXT
	}
	.text : {			/* Real text segment		*/
		_stext = .;		/* Text and read-only data	*/
			__exception_text_start = .;
			*(.exception.text)
			__exception_text_end = .;
			*(.text.hot .text .text.[0-9a-zA-Z_]* .text.fixup .text.unlikely)
			*(.fixup)
			*(.gnu.warning)
		. = ALIGN(16);
		*(.got)			/* Global offset table		*/
	}

	_etext = .;			/* End of text section */

	. = ALIGN(4);
	.altinstructions : {
		__alt_instructions = .;
		*(.altinstructions)
		__alt_instructions_end = .;
	}

	.rela.dyn : ALIGN(8) {
		*(.rela .rela*)
	}

	.data : AT(ADDR(.data) - 0) {
		*(.data .data.[0-9a-zA-Z_]* .data..LPBX*)
	}
	NOTES
	. = ALIGN(8);						\
	.bss : AT(ADDR(.bss) - 0) {				\
		BSS_FIRST_SECTIONS					\
		*(.bss..page_aligned)					\
		*(.dynbss)						\
		*(BSS_MAIN)						\
		*(COMMON)						\
	}
}