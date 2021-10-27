#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>
#include <minix_rt/slab.h>

#include <uapi/minix_rt/binfmt.h>

void system_exec(endpoint_t ep, message_t *m)
{
	int ret, i;
	struct task_struct *exec_tsk, *src_tsk;
	struct minix_rt_binprm *bprm;
	struct pt_regs *regs;
	char *filename, *basename;
	struct mm_struct *mm, *old_mm;
	struct vm_area_struct *vma;
	struct mmap_binprm_info binprm_info, *next;
	void *file, *tmp_buf;
	unsigned long stack_size = THREAD_SIZE;
	unsigned long mmap_off, mmap_size, mmap_base, off;

	src_tsk = pid_find_process_by_pid(m->m_source);
	BUG_ON(!src_tsk);

	bprm = kmalloc(sizeof (struct minix_rt_binprm),
						GFP_KERNEL | GFP_ZERO);
	if (!bprm) {
		m->m_sys_exec.retval = -ENOMEM;
		return;
	}

	ret = mmap_memcpy_from_vma(bprm, m->m_sys_exec.bprm,
				sizeof (struct minix_rt_binprm), src_tsk);
	if (ret != sizeof (struct minix_rt_binprm)) {
		m->m_sys_exec.retval = -EINVAL;
		goto free_bprm;
	}

	exec_tsk = pid_find_process_by_pid(bprm->pid);
	if (!exec_tsk) {
		m->m_sys_exec.retval = -ESRCH;
		goto free_bprm;
	}

	regs = task_pt_regs(exec_tsk);
	if (!in_syscall(regs) || exec_tsk->mm == &init_mm ||
			exec_tsk->flags & PF_SYSTEMSERVICE) {
		m->m_sys_exec.retval = -EIO;
		goto free_bprm;
	}

	sched_exec();

	filename = kmalloc(bprm->filename_size,
					GFP_KERNEL | GFP_ZERO);
	if (!filename) {
		m->m_sys_exec.retval = -ENOMEM;
		goto free_bprm;
	}
	ret = mmap_memcpy_from_vma(filename,
				(unsigned long)bprm->filename,
				bprm->filename_size, src_tsk);
	if (ret != bprm->filename_size) {
		m->m_sys_exec.retval = -EINVAL;
		goto free_filename;
	}

	basename = strrchr(filename, '/');
	if (basename)
		basename++;
	else
		basename = filename;

	file = kmalloc(bprm->file_size, GFP_KERNEL | GFP_ZERO);
	if (!file) {
		m->m_sys_exec.retval = -ENOMEM;
		goto free_filename;
	}
	ret = mmap_memcpy_from_vma(file,
				(unsigned long)bprm->file,
				bprm->file_size, src_tsk);
	if (ret != bprm->file_size) {
		m->m_sys_exec.retval = -EINVAL;
		goto free_file;
	}

	mm = mmap_alloc_mm_struct();
	if (!mm) {
		m->m_sys_exec.retval = -ENOMEM;
		goto free_file;
	}

	next = bprm->binprm_info;
	while (next) {
		ret = mmap_memcpy_from_vma(&binprm_info,
					(unsigned long)next,
					sizeof (struct mmap_binprm_info), src_tsk);
		if (ret != sizeof (struct mmap_binprm_info)) {
			m->m_sys_exec.retval = -EINVAL;
			goto free_filename;
		}
		if (binprm_info.vaddr + binprm_info.size > USER_DS) {
			m->m_sys_exec.retval = -EACCES;
			goto free_filename;
		}
		vma = mmap_get_vmap_area(binprm_info.vaddr, binprm_info.size,
						binprm_info.prot, mm, 0);
		if (!vma) {
			m->m_sys_exec.retval = -EINVAL;
			goto free_mm;
		}

		for (i = 0; i < vma->nr_pages; i++) {
			void *srcaddr = page_to_virt(vma->pages[i]);
			void *dstaddr = file + binprm_info.off + i * PAGE_SIZE;
			memcpy(srcaddr, dstaddr, PAGE_SIZE);
		}
		i = vmap_page_range(vma);
		if (i <= 0) {
			mmap_free_vmap_area(binprm_info.vaddr, mm);
			m->m_sys_exec.retval = -ENOMEM;
			goto free_mm;
		}
		next = binprm_info.next;
	}

	if ((TASK_SIZE - bprm->p) > PAGE_SIZE)
		stack_size += PAGE_ALIGN(TASK_SIZE - bprm->p);

	mm->mmap_base = mm->mmap_end = STACK_TOP - stack_size;
	vma = mmap_get_vmap_area(mm->mmap_base, stack_size,
						VM_READ | VM_WRITE | VM_USER_STACK, mm, 0);
	if (!vma) {
			m->m_sys_exec.retval = -EINVAL;
			goto free_mm;
	}
	i = vmap_page_range(vma);
	if (i <= 0) {
		mmap_free_vmap_area(mm->mmap_base, mm);
		m->m_sys_exec.retval = -ENOMEM;
		goto free_mm;
	}

	tmp_buf = kmalloc(TASK_SIZE - bprm->p, GFP_KERNEL | GFP_ZERO);
	if (!tmp_buf) {
		m->m_sys_exec.retval = -ENOMEM;
		goto free_mm;
	}

	ret = mmap_memcpy_from_vma(tmp_buf,
				(unsigned long)bprm->malloc_p,
				TASK_SIZE - bprm->p, src_tsk);
	if (ret != (TASK_SIZE - bprm->p)) {
		m->m_sys_exec.retval = -EINVAL;
		goto free_tmp_buf;
	}

	mm->mmap_base = mm->mmap_base - PAGE_SIZE;
	vma = mmap_get_vmap_area(mm->mmap_base, PAGE_SIZE,
						VM_READ | VM_WRITE, mm, 0);
	if (!vma) {
		m->m_sys_exec.retval = -EINVAL;
		goto free_tmp_buf;
	}
	i = vmap_page_range(vma);
	if (i <= 0) {
		mmap_free_vmap_area(mm->mmap_base, mm);
		m->m_sys_exec.retval = -ENOMEM;
		goto free_tmp_buf;
	}

	mmap_off = *(u64 *)(tmp_buf +8) - bprm->p;
	mmap_size = TASK_SIZE - *(u64 *)(tmp_buf +8);


	memcpy(page_to_virt(vma->pages[0]), tmp_buf + mmap_off, mmap_size);

	exec_tsk->mm->arg_start = mm->mmap_base;
	mmap_base = *(unsigned long *)(tmp_buf + 8);
	for (i = 0; i < bprm->argc + 1; i++) {
		off = *(u64 *)(tmp_buf +8 + i * sizeof (void *)) - mmap_base;
		*(u64 *)(tmp_buf +8 + i * sizeof (void *)) = mm->mmap_base + off;
	}

	exec_tsk->mm->arg_end = mm->mmap_base +
				*(u64 *)(tmp_buf +8 + (i - 1) * sizeof (void *)) - mmap_base;
	i++;
	bprm->envc += i;
	exec_tsk->mm->env_start = mm->mmap_base +
				*(u64 *)(tmp_buf +8 + i * sizeof (void *)) - mmap_base;

	for (; i < bprm->envc; i++) {
		off = *(u64 *)(tmp_buf +8 + i * sizeof (void *)) - mmap_base;
		*(u64 *)(tmp_buf +8 + i * sizeof (void *)) = mm->mmap_base + off;
	}
	exec_tsk->mm->env_end = mm->mmap_base +
				*(u64 *)(tmp_buf +8 + (i - 1) * sizeof (void *)) - mmap_base;

	old_mm = exec_tsk->mm;
	exec_tsk->mm = mm;

	ret = mmap_memcpy_to_vma(bprm->p,
				TASK_SIZE - bprm->p,
				tmp_buf, exec_tsk);
	if (ret != (TASK_SIZE - bprm->p)) {
		m->m_sys_exec.retval = -EINVAL;
		exec_tsk->mm = old_mm;
		goto free_tmp_buf;
	}

	memset(exec_tsk->comm, 0, sizeof (exec_tsk->comm));
	strlcpy(exec_tsk->comm, basename, sizeof (exec_tsk->comm));
	exec_tsk->mm->start_code = bprm->start_code;
	exec_tsk->mm->end_code = bprm->end_code;
	exec_tsk->mm->start_data = bprm->start_data;
	exec_tsk->mm->end_data = bprm->end_data;
	exec_tsk->mm->elf_brk = bprm->brk;
	exec_tsk->mm->elf_bss = bprm->bss;
	exec_tsk->mm->start_brk = exec_tsk->mm->brk = PAGE_ALIGN(bprm->brk);
	exec_tsk->mm->start_stack = STACK_TOP;

	start_thread(regs, bprm->e_entry, bprm->p);
	regs->regs[0] = 0;
	m->m_sys_exec.retval = 0;

free_tmp_buf:
	kfree(tmp_buf);
free_mm:
	if (m->m_sys_exec.retval) {
		mmap_destroy_mm(mm);
		mmap_free_mm_struct(mm);
	} else if (old_mm) {
		mmap_destroy_mm(old_mm);
		mmap_free_mm_struct(old_mm);
	}
free_file:
	kfree(file);
free_filename:
	kfree(filename);
free_bprm:
	kfree(bprm);
}
