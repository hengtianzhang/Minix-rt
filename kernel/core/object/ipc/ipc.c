#include <sel4m/sched.h>
#include <sel4m/object/untype.h>

int ipc_create_task_ipcptr(struct task_struct *tsk, unsigned long __user ipcptr)
{
	unsigned long flags;
	struct vm_area_struct *vma;

	if (!tsk)
		return -EINVAL;

	flags = VM_READ | VM_WRITE | VM_USER_IPCPTR;
	vma = untype_get_vmap_area(ipcptr, PAGE_SIZE, flags, tsk->mm, 0);
	if (!vma)
		return -ENOMEM;

	if (vmap_page_range(vma) <= 0)
		return -ENOMEM;

	tsk->cap_ipcptr = (void *)ipcptr;

	return 0;
}
