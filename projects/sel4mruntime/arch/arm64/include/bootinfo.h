#ifndef __SEL4MRUNTIME_BOOTINFO_H_
#define __SEL4MRUNTIME_BOOTINFO_H_

#include <base/linkage.h>

asmlinkage void __sel4m_start_c(unsigned long ipcptr);

extern char __executable_start[];
extern char __rela_iplt_start[];
extern char __rela_iplt_end[];
extern char __etext[];
extern char _etext[];
extern char etext[];
extern char __tdata_start[];
extern char __preinit_array_start[];
extern char __preinit_array_end[];
extern char __init_array_start[];
extern char __init_array_end[];
extern char __fini_array_start[];
extern char __fini_array_end[];
extern char __data_start[];
extern char _edata[];
extern char __bss_start[];
extern char __bss_start__[];
extern char _bss_end__[];
extern char __bss_end__[];
extern char __end__[];
extern char _end[];
extern char end[];

#endif /* !__SEL4MRUNTIME_BOOTINFO_H_ */
