#ifndef __UAPI_SEL4M_OBJECT_UNTYPE_H_
#define __UAPI_SEL4M_OBJECT_UNTYPE_H_

enum untype_table {
    untype_alloc,
    untype_free,
    untype_vmap,
    untype_vunmap,
};

/*
 * vm_flags in vm_area_struct, see mm_types.h.
 * When changing, update also include/trace/events/mmflags.h
 */
#define VM_NONE		0x00000000

#define VM_READ		0x00000001	/* currently active flags */
#define VM_WRITE	0x00000002
#define VM_EXEC		0x00000004
#define VM_SHARED	0x00000008

#define VM_IOREMAP	0x00000010

#endif /* !__UAPI_SEL4M_OBJECT_UNTYPE_H_ */
