#include <minix_rt/sched.h>
#include <minix_rt/mmap.h>
#include <minix_rt/mm.h>

int ipc_create_task_ipcptr(struct task_struct *tsk, unsigned long __user ipcptr)
{
	unsigned long flags;
	struct vm_area_struct *vma;

	if (!tsk)
		return -EINVAL;

	flags = VM_READ | VM_WRITE | VM_USER_IPCPTR;
	vma = mmap_get_vmap_area(ipcptr, PAGE_SIZE, flags, tsk->mm, 0);
	if (!vma)
		return -ENOMEM;

	if (vmap_page_range(vma) <= 0)
		return -ENOMEM;

	BUG_ON(vma->nr_pages != 1);

	tsk->cap_ipcptr = (void *)ipcptr;
	tsk->kernel_ipcptr = page_to_virt(vma->pages[0]);

	return 0;
}
