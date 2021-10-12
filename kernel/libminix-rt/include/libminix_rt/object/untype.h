#ifndef __LIBMINIX_RT_OBJECT_UNTYPE_H_
#define __LIBMINIX_RT_OBJECT_UNTYPE_H_

#include <minix_rt/mmap.h>

extern int untype_alloc_area(unsigned long vstart,
            unsigned long size, unsigned long vm_flags);
extern void untype_free_area(unsigned long vstart);

extern int untype_vmap_area(unsigned long vstart);
extern int untype_vunmap_area(unsigned long vstart);

#endif /* !__LIBMINIX_RT_OBJECT_UNTYPE_H_ */
