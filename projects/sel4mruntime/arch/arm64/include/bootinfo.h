#ifndef __SEL4MRUNTIME_BOOTINFO_H_
#define __SEL4MRUNTIME_BOOTINFO_H_

#include <base/linkage.h>

#include <libsel4m/bootinfo.h>

extern struct bootinfo bootinfo;

asmlinkage void __sel4m_start_c(struct bootinfo *bootinfo);

#endif /* !__SEL4MRUNTIME_BOOTINFO_H_ */
