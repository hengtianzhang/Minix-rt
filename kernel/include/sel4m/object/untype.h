#ifndef __SEL4M_OBJECT_H_
#define __SEL4M_OBJECT_H_

#include <base/atomic.h>

#include <asm/pgtable-types.h>
#include <asm/mmu.h>

/*
 * vm_flags in vm_area_struct, see mm_types.h.
 * When changing, update also include/trace/events/mmflags.h
 */
#define VM_NONE		0x00000000

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

struct untype_struct {
	unsigned long nr_pages;
	unsigned long nr_used_pages;
};

extern void untype_core_init(void);
extern void untype_core_init_late(void);


#endif /* !__SEL4M_OBJECT_H_ */
