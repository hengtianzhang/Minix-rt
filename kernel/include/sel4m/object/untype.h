#ifndef __SEL4M_OBJECT_H_
#define __SEL4M_OBJECT_H_

#include <base/atomic.h>

#include <asm/pgtable-types.h>
#include <asm/mmu.h>

struct untype_struct {
	unsigned long nr_pages;
	unsigned long nr_used_pages;
};

extern void untype_core_init(void);
extern void untype_core_init_late(void);


#endif /* !__SEL4M_OBJECT_H_ */
