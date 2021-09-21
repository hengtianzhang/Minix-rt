#include <libsel4m/object/untype.h>
#include <libsel4m/syscalls.h>

int untype_alloc_area(unsigned long vstart,
            unsigned long size, unsigned long vm_flags)
{
    int ret = 0;

    ret = __syscall(__NR_untype, untype_alloc,
            vstart, size, vm_flags);

    return ret;
}

void untype_free_area(unsigned long vstart)
{
   __syscall(__NR_untype, untype_free,
            vstart, 0, 0);
}

int untype_vmap_area(unsigned long vstart)
{
    int ret = 0;

    ret = __syscall(__NR_untype, untype_vmap,
            vstart, 0, 0);

    return ret;
}

int untype_vunmap_area(unsigned long vstart)
{
    int ret = 0;

    ret = __syscall(__NR_untype, untype_vunmap,
            vstart, 0, 0);

    return ret;
}
