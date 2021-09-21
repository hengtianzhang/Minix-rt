#ifndef __LIBSEL4M_OBJECT_UNTYPE_H_
#define __LIBSEL4M_OBJECT_UNTYPE_H_

#include <sel4m/object/untype.h>

extern int untype_alloc_area(unsigned long vstart,
            unsigned long size, unsigned long vm_flags);
extern void untype_free_area(unsigned long vstart);

extern int untype_vmap_area(unsigned long vstart);
extern int untype_vunmap_area(unsigned long vstart);

#endif /* !__LIBSEL4M_OBJECT_UNTYPE_H_ */
