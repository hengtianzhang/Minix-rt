#include <generated/autoconf.h>

#include <base/linkage.h>

#include <asm-generic/base/generic-linker.lds.h>

#include <asm/base/cache.h>
#include <asm/base/page-def.h>

OUTPUT_ARCH(aarch64)
ENTRY(_text)

SECTIONS
{
	/DISCARD/ : {
		*(.discard)
		*(.discard.*)
		*(.interp .dynamic)
		*(.dynsym .dynstr .hash .gnu.hash)
		*(.eh_frame)
	}

	. = 0x400000;

	.text : {
		_text = .;
		_stext = .;
		. = ALIGN(8);
			HEAD_TEXT
			TEXT_TEXT
			*(.fixup)
			*(.gnu.warning)
		. = ALIGN(16);
		*(.got)
	}

	. = ALIGN(PAGE_SIZE);
	_etext = .;

	RO_DATA(PAGE_SIZE)
	NOTES

	_data = .;
	_sdata = .;
	RW_DATA_SECTION(L1_CACHE_BYTES, PAGE_SIZE)
	_edata = .;

	BSS_SECTION(0, 0, 0)

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
