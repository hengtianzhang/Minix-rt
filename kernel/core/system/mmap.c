#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>
#include <minix_rt/slab.h>
#include <minix_rt/of_fdt.h>

static void mmap_initrd(endpoint_t ep, message_t *m, struct task_struct *tsk)
{
	int ret;
	unsigned long vm_flags = m->m_sys_mmap.vm_flags;
	struct vm_area_struct *vma;
	unsigned long vm_start, mmap_start, mmap_size;

	if (!phys_initrd_size) {
		m->m_sys_mmap.retval = -ENXIO;
		return;
	}

	mmap_start = phys_initrd_start & PAGE_MASK;
	mmap_size = PAGE_ALIGN(phys_initrd_size);

	vm_start = PAGE_ALIGN_DOWN(tsk->mm->mmap_base);
	vm_start -= mmap_size;
	vm_flags |= VM_PRIVATE_SHARE;

	vma = mmap_get_vmap_area(vm_start, mmap_size, vm_flags, tsk->mm, mmap_start);
	if (!vma) {
		m->m_sys_mmap.retval = -EINVAL;
		return;
	}
	ret = vmap_page_range(vma);
	if (ret <= 0) {
		mmap_free_vmap_area(vm_start, tsk->mm);
		m->m_sys_mmap.retval = -ENOMEM;
		return;
	}

	tsk->mm->mmap_base = vm_start;
	m->m_sys_mmap.mmap_base = vm_start + offset_in_page(phys_initrd_start);
	m->m_sys_mmap.size = phys_initrd_size;
	m->m_sys_mmap.retval = 0;
}

void system_mmap(endpoint_t ep, message_t *m)
{
	int flags;
	struct task_struct *tsk;

	tsk = pid_find_process_by_pid(m->m_source);
	BUG_ON(!tsk);

	if (tsk->mm == &init_mm) {
		m->m_sys_mmap.retval = -EINVAL;
		return;
	}

	flags = m->m_sys_mmap.flags;
	switch (flags) {
		case MMAP_INITRD:
			mmap_initrd(ep, m, tsk);
			break;
		default:
			m->m_sys_mmap.retval = -ENOSYS;
			break;
	}
}
