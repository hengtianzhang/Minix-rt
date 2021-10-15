#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>

static inline void brk_populate_mess(message_t *m, int state, u64 brk)
{
	m->m_sys_brk.state = state;
	m->m_sys_brk.brk = brk;
}

static int brk_mmap_heap(unsigned long start, unsigned long size,
					unsigned long flags, struct task_struct *tsk)
{
	int ret;
	struct vm_area_struct *vma;

	vma = mmap_get_vmap_area(start, size, flags, tsk->mm, 0);
	if (!vma)
		return -ENOMEM;

	ret = vmap_page_range(vma);
	if (ret <= 0) {
		mmap_free_vmap_area(start, tsk->mm);
		return -ENOMEM;
	}

	return 0;
}

void system_brk(endpoint_t ep, message_t *m)
{
	int m_type = m->m_type & IPC_M_TYPE_MASK;
	struct task_struct *tsk;
	struct vm_area_struct *vma;
	unsigned long start, size;
	unsigned long curr_brk;
	unsigned long mess_brk;
	long increment;
	unsigned long this_start;

	tsk = pid_find_process_by_pid(m->m_source);
	BUG_ON(!tsk);

	switch (m_type) {
		case IPC_M_TYPE_SBRK:
			increment = m->m_sys_brk.brk;
			if (increment < 0)
				brk_populate_mess(m, -1, 0);
			else {
				if (tsk->mm->brk + increment >= tsk->mm->mmap_base) {
					brk_populate_mess(m, -1, 0);

					return ;
				}

				vma = mmap_find_vma_area(tsk->mm->brk + increment, tsk->mm);
				if (vma) {
					tsk->mm->brk += increment;
					brk_populate_mess(m, 0, tsk->mm->brk);
				} else {
					int ret;

					this_start = tsk->mm->brk;
					if (increment == 0) {
						brk_populate_mess(m, 0, tsk->mm->brk);
						return ;
					}

					if (this_start == tsk->mm->start_brk || this_start == tsk->mm->brk)
						this_start -= 1;
	
					vma =  mmap_find_vma_area(this_start, tsk->mm);
					BUG_ON(!vma);

					start = vma->vm_end;
					BUG_ON(!PAGE_ALIGNED(start));

					size = PAGE_ALIGN(tsk->mm->brk + increment - start);
					ret = brk_mmap_heap(start, size, VM_READ | VM_WRITE, tsk);
					if (ret) {
						brk_populate_mess(m, ret, 0);
						return ;
					}
					tsk->mm->brk += increment;
					brk_populate_mess(m, 0, tsk->mm->brk);
				}
			}
			break;
		case IPC_M_TYPE_BRK:
			curr_brk = tsk->mm->brk;
			mess_brk = m->m_sys_brk.brk;
			if (mess_brk < tsk->mm->start_brk ||
					mess_brk >= tsk->mm->mmap_base) {
				brk_populate_mess(m, -EINVAL, 0);

				return ;
			}

			vma = mmap_find_vma_area(mess_brk, tsk->mm);
			if (mess_brk == curr_brk) {
				brk_populate_mess(m, 0, 0);
			} else if (mess_brk > curr_brk) {
				if (vma) {
					tsk->mm->brk = mess_brk;
					brk_populate_mess(m, 0, 0);
				} else {
					int ret;

					this_start = curr_brk;
					if (this_start == tsk->mm->start_brk)
						this_start -= 1;

					vma =  mmap_find_vma_area(this_start, tsk->mm);
					BUG_ON(!vma);

					start = vma->vm_end;
					BUG_ON(!PAGE_ALIGNED(start));

					size = PAGE_ALIGN(mess_brk - start);
					if (!size) {
						brk_populate_mess(m, 0, 0);

						return ;
					}
					ret = brk_mmap_heap(start, size, VM_READ | VM_WRITE, tsk);
					if (ret) {
						brk_populate_mess(m, ret, 0);
						return ;
					}
					tsk->mm->brk = mess_brk;
					brk_populate_mess(m, 0, 0);
				}
			} else {
				struct vm_area_struct *next_vma, *tmp_vma;
				BUG_ON(!vma);

				for_each_next_vm_area_safe(next_vma, tmp_vma, vma) {
					if (next_vma->vm_end >= tsk->mm->mmap_base)
						break;
					vumap_page_range(next_vma);
					mmap_free_vmap_area(next_vma->vm_start, tsk->mm);
				}

				if (vma->vm_start == mess_brk) {
					vumap_page_range(vma);
					mmap_free_vmap_area(vma->vm_start, tsk->mm);
				}

				tsk->mm->brk = mess_brk;
				brk_populate_mess(m, 0, 0);
			}
			break;
		default:
			break;
	}
}
