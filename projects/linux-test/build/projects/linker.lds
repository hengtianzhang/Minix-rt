OUTPUT_ARCH(aarch64)
ENTRY(_start)
SECTIONS
{
 PROVIDE (__executable_start = SEGMENT_START("text-segment", 0x400000));
 . = SEGMENT_START("text-segment", 0x400000) + SIZEOF_HEADERS;
 .interp : { *(.interp) }
 .note.gnu.build-id : { *(.note.gnu.build-id) }
 .hash : { *(.hash) }
 .gnu.hash : { *(.gnu.hash) }
 .dynsym : { *(.dynsym) }
 .dynstr : { *(.dynstr) }
 .gnu.version : { *(.gnu.version) }
 .gnu.version_d : { *(.gnu.version_d) }
 .gnu.version_r : { *(.gnu.version_r) }
 .rela.dyn : {
  *(.rela.init)
  *(.rela.text .rela.text.* .rela.gnu.linkonce.t.*)
  *(.rela.fini)
  *(.rela.rodata .rela.rodata.* .rela.gnu.linkonce.r.*)
  *(.rela.data .rela.data.* .rela.gnu.linkonce.d.*)
  *(.rela.tdata .rela.tdata.* .rela.gnu.linkonce.td.*)
  *(.rela.tbss .rela.tbss.* .rela.gnu.linkonce.tb.*)
  *(.rela.ctors)
  *(.rela.dtors)
  *(.rela.got)
  *(.rela.bss .rela.bss.* .rela.gnu.linkonce.b.*)
  *(.rela.ifunc)
 }
 .rela.plt : {
  *(.rela.plt)
  PROVIDE_HIDDEN (__rela_iplt_start = .);
  *(.rela.iplt)
  PROVIDE_HIDDEN (__rela_iplt_end = .);
 }
 .init : {
  KEEP (*(SORT_NONE(.init)))
 }
 .plt : ALIGN(16) { *(.plt) *(.iplt) }
 .text : {
  *(.text.unlikely .text.*_unlikely .text.unlikely.*)
  *(.text.exit .text.exit.*)
  *(.text.startup .text.startup.*)
  *(.text.hot .text.hot.*)
  *(SORT(.text.sorted.*))
  *(.text .stub .text.* .gnu.linkonce.t.*)
  *(.gnu.warning)
 }
 .fini : {
  KEEP (*(SORT_NONE(.fini)))
 }
 PROVIDE (__etext = .);
 PROVIDE (_etext = .);
 PROVIDE (etext = .);
 .rodata : { *(.rodata .rodata.* .gnu.linkonce.r.*) }
 .rodata1 : { *(.rodata1) }
 .eh_frame_hdr : { *(.eh_frame_hdr) *(.eh_frame_entry .eh_frame_entry.*) }
 .eh_frame : ONLY_IF_RO { KEEP (*(.eh_frame)) *(.eh_frame.*) }
 .gcc_except_table : ONLY_IF_RO { *(.gcc_except_table .gcc_except_table.*) }
 .gnu_extab : ONLY_IF_RO { *(.gnu_extab*) }
 .exception_ranges : ONLY_IF_RO { *(.exception_ranges*) }
 . = DATA_SEGMENT_ALIGN (CONSTANT (MAXPAGESIZE), CONSTANT (COMMONPAGESIZE));
 .eh_frame : ONLY_IF_RW { KEEP (*(.eh_frame)) *(.eh_frame.*) }
 .gnu_extab : ONLY_IF_RW { *(.gnu_extab) }
 .gcc_except_table : ONLY_IF_RW { *(.gcc_except_table .gcc_except_table.*) }
 .exception_ranges : ONLY_IF_RW { *(.exception_ranges*) }
 .tdata : {
  PROVIDE_HIDDEN (__tdata_start = .);
  *(.tdata .tdata.* .gnu.linkonce.td.*)
 }
 .tbss : { *(.tbss .tbss.* .gnu.linkonce.tb.*) *(.tcommon) }
 .preinit_array : {
  PROVIDE_HIDDEN (__preinit_array_start = .);
  KEEP (*(.preinit_array))
  PROVIDE_HIDDEN (__preinit_array_end = .);
 }
 .init_array : {
  PROVIDE_HIDDEN (__init_array_start = .);
  KEEP (*(SORT_BY_INIT_PRIORITY(.init_array.*) SORT_BY_INIT_PRIORITY(.ctors.*)))
  KEEP (*(.init_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .ctors))
  PROVIDE_HIDDEN (__init_array_end = .);
 }
 .fini_array : {
  PROVIDE_HIDDEN (__fini_array_start = .);
  KEEP (*(SORT_BY_INIT_PRIORITY(.fini_array.*) SORT_BY_INIT_PRIORITY(.dtors.*)))
  KEEP (*(.fini_array EXCLUDE_FILE (*crtbegin.o *crtbegin?.o *crtend.o *crtend?.o ) .dtors))
  PROVIDE_HIDDEN (__fini_array_end = .);
 }
 .ctors : {
  KEEP (*crtbegin.o(.ctors))
  KEEP (*crtbegin?.o(.ctors))
  KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .ctors))
  KEEP (*(SORT(.ctors.*)))
  KEEP (*(.ctors))
 }
 .dtors : {
  KEEP (*crtbegin.o(.dtors))
  KEEP (*crtbegin?.o(.dtors))
  KEEP (*(EXCLUDE_FILE (*crtend.o *crtend?.o ) .dtors))
  KEEP (*(SORT(.dtors.*)))
  KEEP (*(.dtors))
 }
 .jcr : { KEEP (*(.jcr)) }
 .data.rel.ro : { *(.data.rel.ro.local* .gnu.linkonce.d.rel.ro.local.*) *(.data.rel.ro .data.rel.ro.* .gnu.linkonce.d.rel.ro.*) }
 .dynamic : { *(.dynamic) }
 .got : { *(.got) *(.igot) }
 . = DATA_SEGMENT_RELRO_END (24, .);
 .got.plt : { *(.got.plt) *(.igot.plt) }
 .data : {
  PROVIDE (__data_start = .);
  *(.data .data.* .gnu.linkonce.d.*)
  SORT(CONSTRUCTORS)
 }
 .data1 : { *(.data1) }
 _edata = .; PROVIDE (edata = .);
 . = .;
 __bss_start = .;
 __bss_start__ = .;
 .bss : {
 *(.dynbss)
 *(.bss .bss.* .gnu.linkonce.b.*)
 *(COMMON)
 . = ALIGN(. != 0 ? 64 / 8 : 1);
 }
 _bss_end__ = .; __bss_end__ = .;
 . = ALIGN(64 / 8);
 . = SEGMENT_START("ldata-segment", .);
 . = ALIGN(64 / 8);
 __end__ = .;
 _end = .; PROVIDE (end = .);
 . = DATA_SEGMENT_END (.);
 .stab 0 : { *(.stab) }
 .stabstr 0 : { *(.stabstr) }
 .stab.excl 0 : { *(.stab.excl) }
 .stab.exclstr 0 : { *(.stab.exclstr) }
 .stab.index 0 : { *(.stab.index) }
 .stab.indexstr 0 : { *(.stab.indexstr) }
 .comment 0 : { *(.comment) }
 .gnu.build.attributes : { *(.gnu.build.attributes .gnu.build.attributes.*) }
 .debug 0 : { *(.debug) }
 .line 0 : { *(.line) }
 .debug_srcinfo 0 : { *(.debug_srcinfo) }
 .debug_sfnames 0 : { *(.debug_sfnames) }
 .debug_aranges 0 : { *(.debug_aranges) }
 .debug_pubnames 0 : { *(.debug_pubnames) }
 .debug_info 0 : { *(.debug_info .gnu.linkonce.wi.*) }
 .debug_abbrev 0 : { *(.debug_abbrev) }
 .debug_line 0 : { *(.debug_line .debug_line.* .debug_line_end) }
 .debug_frame 0 : { *(.debug_frame) }
 .debug_str 0 : { *(.debug_str) }
 .debug_loc 0 : { *(.debug_loc) }
 .debug_macinfo 0 : { *(.debug_macinfo) }
 .debug_weaknames 0 : { *(.debug_weaknames) }
 .debug_funcnames 0 : { *(.debug_funcnames) }
 .debug_typenames 0 : { *(.debug_typenames) }
 .debug_varnames 0 : { *(.debug_varnames) }
 .debug_pubtypes 0 : { *(.debug_pubtypes) }
 .debug_ranges 0 : { *(.debug_ranges) }
 .debug_macro 0 : { *(.debug_macro) }
 .debug_addr 0 : { *(.debug_addr) }
 .ARM.attributes 0 : { KEEP (*(.ARM.attributes)) KEEP (*(.gnu.attributes)) }
 .note.gnu.arm.ident 0 : { KEEP (*(.note.gnu.arm.ident)) }
 /DISCARD/ : { *(.note.GNU-stack) *(.gnu_debuglink) *(.gnu.lto_*) }
}
