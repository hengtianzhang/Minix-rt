
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

OUTPUT_ARCH(aarch64)
ENTRY(_text)

SECTIONS
{
	. = 0x400000;

	_text = .;
	.text : {
		. = ALIGN(8);
			_stext = .;
			*(.text.hot .text .text.[0-9a-zA-Z_]* .text.fixup .text.unlikely)
			*(.fixup)
			*(.gnu.warning)
		. = ALIGN(16);
		*(.got)
	}

	. = ALIGN(0x10000);
	_etext = .;
	. = ALIGN(4096);

	_sdata = .;
	.rodata : {
		_srodata = .;
		*(.rodata) *(.rodata.*)
		*(.rodata1)
	}
	. = ALIGN(4096);
	_erodata = .;

	.rela.dyn : ALIGN(8) {
		*(.rela .rela*)
	}

	.data : {
		*(.data .data.[0-9a-zA-Z_]* .data..LPBX*)
	}
	.note : {
		_snotes = .;
		KEEP(*(.note.*))
		_enotes = .;
	}

	__bss_start = .;
	.sbss : {
		*(.dynbss)
		*(.sbss .sbss.[0-9a-zA-Z_]*)
		*(.scommon)
	}
	.bss : {
		*(.dynbss)
		*(.bss .bss.[0-9a-zA-Z_]*)
		*(COMMON)
	}
	__bss_end = .;

	_end = .;

	STABS_DEBUG
	DWARF_DEBUG
	ELF_DETAILS

	/*
	 * Sections that should stay zero sized, which is safer to
	 * explicitly check instead of blindly discarding.
	 */
	.plt : {
		*(*.plt) *(.plt) *(.plt.*) *(.iplt) *(.igot .igot.plt) 
	}
}
