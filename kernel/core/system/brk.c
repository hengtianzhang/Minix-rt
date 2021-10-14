#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>

void system_brk(endpoint_t ep, message_t *m)
{
	struct task_struct *tsk;
	unsigned long start;
	long size, heap_size;
	int ret;
	struct vm_area_struct *vma;

	tsk = pid_find_process_by_pid(m->m_source);
	BUG_ON(!tsk);

	if (m->m_type == IPC_M_TYPE_SBRK) {
		size = m->m_sys_brk.brk;
		if (size < 0) {
			m->m_sys_brk.state = -1;
			m->m_sys_brk.brk = 0;
		} else if (size == 0) {
			m->m_sys_brk.state = 0;
			m->m_sys_brk.brk = tsk->mm->brk;
		} else {
			if (PAGE_ALIGN(tsk->mm->brk) > tsk->mm->brk + size) {
				tsk->mm->brk += size;
				m->m_sys_brk.state = 0;
				m->m_sys_brk.brk = tsk->mm->brk;
			} else {
				start = PAGE_ALIGN(tsk->mm->brk);
				heap_size = PAGE_ALIGN(size - PAGE_ALIGN(tsk->mm->brk) + tsk->mm->brk);
				vma = mmap_get_vmap_area(start, heap_size, VM_READ | VM_WRITE, tsk->mm, 0);
				if (!vma) {
					m->m_sys_brk.state = -ENOMEM;
					m->m_sys_brk.brk = 0;

					return ;
				}
				ret = vmap_page_range(vma);
				if (ret <= 0) {
					mmap_free_vmap_area(start, tsk->mm);
					m->m_sys_brk.state = -ENOMEM;
					m->m_sys_brk.brk = 0;

					return ;
				}
				tsk->mm->brk += size;
				m->m_sys_brk.state = 0;
				m->m_sys_brk.brk = tsk->mm->brk;
			}
		}
	} else if (m->m_type == IPC_M_TYPE_BRK) {
		unsigned long curr_brk;

		start = m->m_sys_brk.brk;
		if (start < tsk->mm->start_brk) {
			m->m_sys_brk.state = -EINVAL;
			m->m_sys_brk.brk = 0;

			return ;
		}

		curr_brk = tsk->mm->brk;
		if (curr_brk == start) {
			m->m_sys_brk.state = 0;
		} else if (start < curr_brk) {
			if (PAGE_ALIGN_DOWN(curr_brk) < start) {
				tsk->mm->brk = start;
				m->m_sys_brk.state = 0;

				return ;
			}

			start = PAGE_ALIGN(start);
			size = PAGE_ALIGN(tsk->mm->brk) - start;
			vma = mmap_find_vma_area(start, tsk->mm);
			BUG_ON(!vma);

			vumap_page_range(vma);
			mmap_free_vmap_area(start, tsk->mm);
			tsk->mm->brk = m->m_sys_brk.brk;
			m->m_sys_brk.state = 0;
		} else if (start > curr_brk) {
			if (PAGE_ALIGN(curr_brk) > start) {
				tsk->mm->brk = start;
				m->m_sys_brk.state = 0;
			} else {
				curr_brk = PAGE_ALIGN(curr_brk);
				size = PAGE_ALIGN(start - curr_brk);
				vma = mmap_get_vmap_area(curr_brk, size, VM_READ | VM_WRITE, tsk->mm, 0);
				if (!vma) {
					m->m_sys_brk.state = -ENOMEM;
					m->m_sys_brk.brk = 0;

					return ;
				}
				ret = vmap_page_range(vma);
				if (ret <= 0) {
					mmap_free_vmap_area(start, tsk->mm);
					m->m_sys_brk.state = -ENOMEM;
					m->m_sys_brk.brk = 0;

					return ;
				}
				tsk->mm->brk = start;
				m->m_sys_brk.state = 0;
			}
		}
	}
}
