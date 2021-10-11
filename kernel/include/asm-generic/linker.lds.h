#ifndef __ASM_GENERIC_LINKER_LDS_H_
#define __ASM_GENERIC_LINKER_LDS_H_

#include <base/linkage.h>

#ifndef LOAD_OFFSET
#define LOAD_OFFSET 0
#endif

/* Align . to a 8 byte boundary equals to maximum function alignment. */
#define ALIGN_FUNCTION()  . = ALIGN(8)

/*
 * LD_DEAD_CODE_DATA_ELIMINATION option enables -fdata-sections, which
 * generates .data.identifier sections, which need to be pulled in with
 * .data. We don't want to pull in .data..other sections, which Linux
 * has defined. Same for text and bss.
 *
 * RODATA_MAIN is not used because existing code already defines .rodata.x
 * sections to be brought in with rodata.
 */
#ifdef CONFIG_LD_DEAD_CODE_DATA_ELIMINATION
#define TEXT_MAIN .text .text.[0-9a-zA-Z_]*
#define DATA_MAIN .data .data.[0-9a-zA-Z_]* .data..LPBX*
#define SDATA_MAIN .sdata .sdata.[0-9a-zA-Z_]*
#define RODATA_MAIN .rodata .rodata.[0-9a-zA-Z_]*
#define BSS_MAIN .bss .bss.[0-9a-zA-Z_]*
#define SBSS_MAIN .sbss .sbss.[0-9a-zA-Z_]*
#else
#define TEXT_MAIN .text
#define DATA_MAIN .data
#define SDATA_MAIN .sdata
#define RODATA_MAIN .rodata
#define BSS_MAIN .bss
#define SBSS_MAIN .sbss
#endif

/* Section used for early init (in .S files) */
#define HEAD_TEXT  KEEP(*(.head.text))

#define TEXT_TEXT				\
		ALIGN_FUNCTION();		\
		*(.text.hot TEXT_MAIN .text.fixup .text.unlikely)	\

/* sched.text is aling to function alignment to secure we have same
 * address even at second ld pass when generating System.map */
#define SCHED_TEXT							\
		ALIGN_FUNCTION();					\
		__sched_text_start = .;					\
		*(.sched.text)						\
		__sched_text_end = .;

#define CPUIDLE_TEXT							\
		ALIGN_FUNCTION();					\
		__cpuidle_text_start = .;				\
		*(.cpuidle.text)					\
		__cpuidle_text_end = .;

/* spinlock.text is aling to function alignment to secure we have same
 * address even at second ld pass when generating System.map */
#define LOCK_TEXT							\
		ALIGN_FUNCTION();					\
		__lock_text_start = .;					\
		*(.spinlock.text)					\
		__lock_text_end = .;

#define ENTRY_TEXT							\
		ALIGN_FUNCTION();					\
		__entry_text_start = .;					\
		*(.entry.text)						\
		__entry_text_end = .;

#define IRQENTRY_TEXT							\
		ALIGN_FUNCTION();					\
		__irqentry_text_start = .;				\
		*(.irqentry.text)					\
		__irqentry_text_end = .;

#define INIT_TEXT							\
	*(.init.text .init.text.*)					\
	*(.text.startup)

#define INIT_TEXT_SECTION(inittext_align)				\
	. = ALIGN(inittext_align);					\
	.init.text : AT(ADDR(.init.text) - LOAD_OFFSET) {		\
		_sinittext = .;						\
		INIT_TEXT						\
		_einittext = .;						\
	}

/*
 * Exception table
 */
#define EXCEPTION_TABLE(align)						\
	. = ALIGN(align);						\
	__ex_table : AT(ADDR(__ex_table) - LOAD_OFFSET) {		\
		__start___ex_table = .;					\
		KEEP(*(__ex_table))					\
		__stop___ex_table = .;					\
	}

/*
 * Allow architectures to handle ro_after_init data on their
 * own by defining an empty RO_AFTER_INIT_DATA.
 */
#ifndef RO_AFTER_INIT_DATA
#define RO_AFTER_INIT_DATA						\
	__start_ro_after_init = .;					\
	*(.data..ro_after_init)						\
	__end_ro_after_init = .;
#endif

/*
 * Read only Data
 */
#define RO_DATA_SECTION(align)						\
	. = ALIGN((align));						\
	.rodata : AT(ADDR(.rodata) - LOAD_OFFSET) {		\
		__start_rodata = .;					\
		*(.rodata) *(.rodata.*)					\
		RO_AFTER_INIT_DATA	/* Read only after init */	\
	}				\
					\
	ARCHIVE_SERVICES(PAGE_SIZE)			\
					\
	.rodata1 : AT(ADDR(.rodata1) - LOAD_OFFSET) {		\
		*(.rodata1)						\
		__end_rodata = .;				\
	}				\
					\
	. = ALIGN((align));

#define RO_DATA(align)  RO_DATA_SECTION(align)

#define OF_TABLE(name)						\
	. = ALIGN(8);							\
	__##name##_of_table_start = .;					\
	KEEP(*(__##name##_of_table))					\
	KEEP(*(__##name##_of_table_end))					\
	__##name##_of_table_end = .;

#define CONSOLE_OF_TABLES() OF_TABLE(console)
#define CLK_OF_TABLES()		OF_TABLE(clk)
#define TIMER_OF_TABLES()	OF_TABLE(timer)
#define IRQCHIP_OF_MATCH_TABLE() OF_TABLE(irqchip)

/* init and exit section handling */
#define INIT_DATA							\
	*(.init.data init.data.*)					\
	*(.init.rodata .init.rodata.*)					\
	CONSOLE_OF_TABLES()						\
	CLK_OF_TABLES()							\
	TIMER_OF_TABLES()						\
	IRQCHIP_OF_MATCH_TABLE()

#define ARCHIVE_SERVICES(align)		\
	. = ALIGN((align));						\
	.archive_services :  AT(ADDR(.archive_services) - LOAD_OFFSET) {		\
		__start_archive = .;				\
		KEEP(*(.archive_services))			\
		__end_archive = .;					\
	}

#define PAGE_ALIGNED_DATA(page_align)					\
	. = ALIGN(page_align);						\
	*(.data..page_aligned)

#define CACHELINE_ALIGNED_DATA(align)					\
	. = ALIGN(align);						\
	*(.data..cacheline_aligned)

#define READ_MOSTLY_DATA(align)						\
	. = ALIGN(align);						\
	*(.data..read_mostly)						\
	. = ALIGN(align);

#define KERNEL_STACK_DATA				\
	__start_kernel_stack = .;			\
	KEEP(*(.data..kernel_stack))			\
	__end_kernel_stack	= .;

#define DATA_DATA							\
	*(DATA_MAIN)							\
	__start_once = .;						\
	*(.data.once)							\
	__end_once = .;

#define RW_DATA_SECTION(cacheline, pagealigned)		\
	. = ALIGN(PAGE_SIZE);						\
	.data : AT(ADDR(.data) - LOAD_OFFSET) {				\
		PAGE_ALIGNED_DATA(pagealigned)				\
		CACHELINE_ALIGNED_DATA(cacheline)			\
		READ_MOSTLY_DATA(cacheline)				\
		KERNEL_STACK_DATA					\
		DATA_DATA						\
		CONSTRUCTORS						\
	}								\

/*
 * bss (Block Started by Symbol) - uninitialized data
 * zeroed during startup
 */
#define SBSS(sbss_align)						\
	. = ALIGN(sbss_align);						\
	.sbss : AT(ADDR(.sbss) - LOAD_OFFSET) {				\
		*(.dynsbss)						\
		*(SBSS_MAIN)						\
		*(.scommon)						\
	}

#define BSS(bss_align)							\
	. = ALIGN(bss_align);						\
	.bss : AT(ADDR(.bss) - LOAD_OFFSET) {				\
		*(.bss..page_aligned)					\
		*(.dynbss)						\
		*(BSS_MAIN)						\
		*(COMMON)						\
	}

#define BSS_SECTION(sbss_align, bss_align, stop_align)			\
	. = ALIGN(sbss_align);						\
	__bss_start = .;						\
	SBSS(sbss_align)						\
	BSS(bss_align)							\
	. = ALIGN(stop_align);						\
	__bss_stop = .;

#define NOTES								\
	.notes : AT(ADDR(.notes) - LOAD_OFFSET) {			\
		__start_notes = .;					\
		KEEP(*(.note.*))					\
		__stop_notes = .;					\
	}

/* Stabs debugging sections.  */
#define STABS_DEBUG							\
		.stab 0 : { *(.stab) }					\
		.stabstr 0 : { *(.stabstr) }				\
		.stab.excl 0 : { *(.stab.excl) }			\
		.stab.exclstr 0 : { *(.stab.exclstr) }			\
		.stab.index 0 : { *(.stab.index) }			\
		.stab.indexstr 0 : { *(.stab.indexstr) }

/*
 * DWARF debug sections.
 * Symbols in the DWARF debugging sections are relative to
 * the beginning of the section so we begin them at 0.
 */
#define DWARF_DEBUG							\
		/* DWARF 1 */						\
		.debug          0 : { *(.debug) }			\
		.line           0 : { *(.line) }			\
		/* GNU DWARF 1 extensions */				\
		.debug_srcinfo  0 : { *(.debug_srcinfo) }		\
		.debug_sfnames  0 : { *(.debug_sfnames) }		\
		/* DWARF 1.1 and DWARF 2 */				\
		.debug_aranges  0 : { *(.debug_aranges) }		\
		.debug_pubnames 0 : { *(.debug_pubnames) }		\
		/* DWARF 2 */						\
		.debug_info     0 : { *(.debug_info			\
				.gnu.linkonce.wi.*) }			\
		.debug_abbrev   0 : { *(.debug_abbrev) }		\
		.debug_line     0 : { *(.debug_line) }			\
		.debug_frame    0 : { *(.debug_frame) }			\
		.debug_str      0 : { *(.debug_str) }			\
		.debug_loc      0 : { *(.debug_loc) }			\
		.debug_macinfo  0 : { *(.debug_macinfo) }		\
		.debug_pubtypes 0 : { *(.debug_pubtypes) }		\
		/* DWARF 3 */						\
		.debug_ranges	0 : { *(.debug_ranges) }		\
		/* SGI/MIPS DWARF 2 extensions */			\
		.debug_weaknames 0 : { *(.debug_weaknames) }		\
		.debug_funcnames 0 : { *(.debug_funcnames) }		\
		.debug_typenames 0 : { *(.debug_typenames) }		\
		.debug_varnames  0 : { *(.debug_varnames) }		\
		/* GNU DWARF 2 extensions */				\
		.debug_gnu_pubnames 0 : { *(.debug_gnu_pubnames) }	\
		.debug_gnu_pubtypes 0 : { *(.debug_gnu_pubtypes) }	\
		/* DWARF 4 */						\
		.debug_types	0 : { *(.debug_types) }			\
		/* DWARF 5 */						\
		.debug_macro	0 : { *(.debug_macro) }			\
		.debug_addr	0 : { *(.debug_addr) }

/* Required sections not related to debugging. */
#define ELF_DETAILS							\
		.comment 0 : { *(.comment) }				\
		.symtab 0 : { *(.symtab) }				\
		.strtab 0 : { *(.strtab) }				\
		.shstrtab 0 : { *(.shstrtab) }

#endif /* !__ASM_GENERIC_LINKER_LDS_H_ */
