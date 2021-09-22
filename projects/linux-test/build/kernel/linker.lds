OUTPUT_ARCH(aarch64)
ENTRY(_text)
jiffies = jiffies_64;
SECTIONS
{
 /DISCARD/ : {
  *(.discard)
  *(.discard.*)
  *(.interp .dynamic)
  *(.dynsym .dynstr .hash .gnu.hash)
  *(.eh_frame)
 }
 . = ((((0xffffffffffffffff)) - (((1)) << (48)) + 1));
 .head.text : {
  _text = .;
  KEEP(*(.head.text))
 }
 .text : {
  _stext = .;
   __exception_text_start = .;
   *(.exception.text)
   __exception_text_end = .;
   . = ALIGN(8); __irqentry_text_start = .; *(.irqentry.text) __irqentry_text_end = .;
   . = ALIGN(8); __entry_text_start = .; *(.entry.text) __entry_text_end = .;
   . = ALIGN(8); *(.text.hot .text .text.fixup .text.unlikely)
   . = ALIGN(8); __sched_text_start = .; *(.sched.text) __sched_text_end = .;
   . = ALIGN(8); __cpuidle_text_start = .; *(.cpuidle.text) __cpuidle_text_end = .;
   . = ALIGN(8); __lock_text_start = .; *(.spinlock.text) __lock_text_end = .;
   . = ALIGN(0x00001000); __idmap_text_start = .; *(.idmap.text) __idmap_text_end = .;
   *(.fixup)
   *(.gnu.warning)
  . = ALIGN(16);
  *(.got)
 }
 . = ALIGN(0x00010000);
 _etext = .;
 . = ALIGN(((1 << 12))); .rodata : AT(ADDR(.rodata) - 0) { __start_rodata = .; *(.rodata) *(.rodata.*) __start_ro_after_init = .; *(.data..ro_after_init) __end_ro_after_init = .; } .rodata1 : AT(ADDR(.rodata1) - 0) { *(.rodata1) __end_rodata = .; } . = ALIGN(((1 << 12)));
 . = ALIGN(8); __ex_table : AT(ADDR(__ex_table) - 0) { __start___ex_table = .; KEEP(*(__ex_table)) __stop___ex_table = .; }
 .notes : AT(ADDR(.notes) - 0) { __start_notes = .; KEEP(*(.note.*)) __stop_notes = .; }
 . = ALIGN((1 << 12));
 idmap_pg_dir = .;
 . += ((((((48)) - 4) / (12 - 3)) - 1) * (1 << 12));
 swapper_pg_dir = .;
 . += (1 << 12);
 swapper_pg_end = .;
 . = ALIGN(0x00010000);
 __init_begin = .;
 __inittext_begin = .;
 . = ALIGN(8); .init.text : AT(ADDR(.init.text) - 0) { _sinittext = .; *(.init.text .init.text.*) *(.text.startup) _einittext = .; }
 . = ALIGN(4);
 .altinstructions : {
  __alt_instructions = .;
  *(.altinstructions)
  __alt_instructions_end = .;
 }
 .altinstr_replacement : {
  *(.altinstr_replacement)
 }
 . = ALIGN((1 << 12));
 __inittext_end = .;
 __initdata_begin = .;
 .init.data : {
  *(.init.data init.data.*) *(.init.rodata .init.rodata.*) . = ALIGN(8); __console_of_table_start = .; KEEP(*(__console_of_table)) KEEP(*(__console_of_table_end)) __console_of_table_end = .; . = ALIGN(8); __clk_of_table_start = .; KEEP(*(__clk_of_table)) KEEP(*(__clk_of_table_end)) __clk_of_table_end = .; . = ALIGN(8); __timer_of_table_start = .; KEEP(*(__timer_of_table)) KEEP(*(__timer_of_table_end)) __timer_of_table_end = .; . = ALIGN(8); __irqchip_of_table_start = .; KEEP(*(__irqchip_of_table)) KEEP(*(__irqchip_of_table_end)) __irqchip_of_table_end = .;
  *(.init.rodata.* .init.bss)
 }
 . = ALIGN(((1 << 12))); .archive_services : AT(ADDR(.archive_services) - 0) { __start_archive = .; KEEP(*(.archive_services)) __end_archive = .; }
 .rela.dyn : ALIGN(8) {
  *(.rela .rela*)
 }
 __rela_offset = ABSOLUTE(ADDR(.rela.dyn) - ((((0xffffffffffffffff)) - (((1)) << (48)) + 1)));
 __rela_size = SIZEOF(.rela.dyn);
 . = ALIGN(0x00010000);
 __initdata_end = .;
 __init_end = .;
 _data = .;
 _sdata = .;
 . = ALIGN((1 << 12)); .data : AT(ADDR(.data) - 0) { . = ALIGN((1 << 12)); *(.data..page_aligned) . = ALIGN((1 << (6))); *(.data..cacheline_aligned) . = ALIGN((1 << (6))); *(.data..read_mostly) . = ALIGN((1 << (6))); __start_kernel_stack = .; KEEP(*(.data..kernel_stack)) __end_kernel_stack = .; *(.data) __start_once = .; *(.data.once) __end_once = .; CONSTRUCTORS }
 .mmuoff.data.write : ALIGN(0x00000800) {
  __mmuoff_data_start = .;
  *(.mmuoff.data.write)
 }
 . = ALIGN(0x00000800);
 .mmuoff.data.read : {
  *(.mmuoff.data.read)
  __mmuoff_data_end = .;
 }
 _edata = .;
 . = ALIGN(0); __bss_start = .; . = ALIGN(0); .sbss : AT(ADDR(.sbss) - 0) { *(.dynsbss) *(.sbss) *(.scommon) } . = ALIGN(0); .bss : AT(ADDR(.bss) - 0) { *(.bss..page_aligned) *(.dynbss) *(.bss) *(COMMON) } . = ALIGN(0); __bss_stop = .;
 . = ALIGN((1 << 12));
 init_pg_dir = .;
 . += ((1 << 12) * ( 1 + (((((_end)) >> (((12 - 3) * (4 - (4 - 4)) + 3))) - (((((((0xffffffffffffffff)) - (((1)) << (48)) + 1)) + 0)) >> (((12 - 3) * (4 - (4 - 4)) + 3))) + 1)) + (0) + (((((_end)) >> (((12 - 3) * (4 - (1)) + 3))) - (((((((0xffffffffffffffff)) - (((1)) << (48)) + 1)) + 0)) >> (((12 - 3) * (4 - (1)) + 3))) + 1))));
 init_pg_end = .;
 _end = .;
 .stab 0 : { *(.stab) } .stabstr 0 : { *(.stabstr) } .stab.excl 0 : { *(.stab.excl) } .stab.exclstr 0 : { *(.stab.exclstr) } .stab.index 0 : { *(.stab.index) } .stab.indexstr 0 : { *(.stab.indexstr) }
 .debug 0 : { *(.debug) } .line 0 : { *(.line) } .debug_srcinfo 0 : { *(.debug_srcinfo) } .debug_sfnames 0 : { *(.debug_sfnames) } .debug_aranges 0 : { *(.debug_aranges) } .debug_pubnames 0 : { *(.debug_pubnames) } .debug_info 0 : { *(.debug_info .gnu.linkonce.wi.*) } .debug_abbrev 0 : { *(.debug_abbrev) } .debug_line 0 : { *(.debug_line) } .debug_frame 0 : { *(.debug_frame) } .debug_str 0 : { *(.debug_str) } .debug_loc 0 : { *(.debug_loc) } .debug_macinfo 0 : { *(.debug_macinfo) } .debug_pubtypes 0 : { *(.debug_pubtypes) } .debug_ranges 0 : { *(.debug_ranges) } .debug_weaknames 0 : { *(.debug_weaknames) } .debug_funcnames 0 : { *(.debug_funcnames) } .debug_typenames 0 : { *(.debug_typenames) } .debug_varnames 0 : { *(.debug_varnames) } .debug_gnu_pubnames 0 : { *(.debug_gnu_pubnames) } .debug_gnu_pubtypes 0 : { *(.debug_gnu_pubtypes) } .debug_types 0 : { *(.debug_types) } .debug_macro 0 : { *(.debug_macro) } .debug_addr 0 : { *(.debug_addr) }
 .comment 0 : { *(.comment) } .symtab 0 : { *(.symtab) } .strtab 0 : { *(.strtab) } .shstrtab 0 : { *(.shstrtab) }
 _kernel_size_le_lo32 = (((_end - _text) & 0xffffffff) & 0xffffffff); _kernel_size_le_hi32 = (((_end - _text) >> 32) & 0xffffffff); _kernel_offset_le_lo32 = (((0) & 0xffffffff) & 0xffffffff); _kernel_offset_le_hi32 = (((0) >> 32) & 0xffffffff); _kernel_flags_le_lo32 = (((((0 << 0) | (((12 - 10) / 2) << (0 + 1)) | (1 << ((0 + 1) + 2)))) & 0xffffffff) & 0xffffffff); _kernel_flags_le_hi32 = (((((0 << 0) | (((12 - 10) / 2) << (0 + 1)) | (1 << ((0 + 1) + 2)))) >> 32) & 0xffffffff);
 .plt : {
  *(.plt) *(.plt.*) *(.iplt) *(.igot .igot.plt)
 }
 ASSERT(SIZEOF(.plt) == 0, "Unexpected run-time procedure linkages detected!")
 .data.rel.ro : { *(.data.rel.ro) }
 ASSERT(SIZEOF(.data.rel.ro) == 0, "Unexpected RELRO detected!")
}
ASSERT(__idmap_text_end - (__idmap_text_start & ~(0x00001000 - 1)) <= 0x00001000,
 "ID map text too big or misaligned")
ASSERT(_text == ((((0xffffffffffffffff)) - (((1)) << (48)) + 1)), "HEAD is misaligned")
