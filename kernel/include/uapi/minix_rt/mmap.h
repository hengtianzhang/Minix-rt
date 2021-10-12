#ifndef __UAPI_MINIX_RT_MMAP_H_
#define __UAPI_MINIX_RT_MMAP_H_

/*
 * vm_flags in vm_area_struct, see mm_types.h.
 * When changing, update also include/trace/events/mmflags.h
 */
#define VM_NONE		    0x00000000

#define VM_READ		    0x00000001	/* currently active flags */
#define VM_WRITE	    0x00000002
#define VM_EXEC		    0x00000004
#define VM_SHARED	    0x00000008

#define VM_IOREMAP  	0x00000010

#define VM_USER_STACK   0x00000020
#define VM_USER_IPCPTR  0x00000040

#define VM_PRIVATE_SHARE    0x00000080

#endif /* !__UAPI_MINIX_RT_MMAP_H_ */
