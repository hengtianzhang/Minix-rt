#include <minix_rt/system.h>
#include <minix_rt/sched.h>
#include <minix_rt/page.h>
#include <minix_rt/mmap.h>
#include <minix_rt/slab.h>

#include <uapi/minix_rt/binfmt.h>

void system_exec(endpoint_t ep, message_t *m)
{
	int ret;
	struct task_struct *exec_tsk, *src_tsk;
	struct minix_rt_binprm *bprm;
	struct pt_regs *regs;
	char *filename, *basename;
	char **argv, **envp;

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

	argv = kmalloc(bprm->argc * sizeof (void *), GFP_KERNEL | GFP_ZERO);

	if (!argv) {
		m->m_sys_exec.retval = -ENOMEM;
		goto free_filename;
	}
	ret = mmap_memcpy_from_vma(argv,
				(unsigned long)bprm->argv,
				bprm->argc * sizeof (void *), exec_tsk);
	if (ret != bprm->argc * sizeof (void *)) {
		m->m_sys_exec.retval = -EINVAL;
		goto free_argv;
	}
	envp = kmalloc(bprm->envc * sizeof (void *), GFP_KERNEL | GFP_ZERO);
	if (!envp) {
		m->m_sys_exec.retval = -ENOMEM;
		goto free_argv;
	}
	ret = mmap_memcpy_from_vma(envp,
				(unsigned long)bprm->envp,
				bprm->envc * sizeof (void *), exec_tsk);
	if (ret != bprm->envc * sizeof (void *)) {
		m->m_sys_exec.retval = -EINVAL;
		goto free_envp;
	}

	printf("ssss %p %p\n", argv[0], envp[0]);

	basename = strrchr(filename, '/');
	if (basename)
		basename++;
	else
		basename = filename;

free_envp:
	kfree(envp);
free_argv:
	kfree(argv);
free_filename:
	kfree(filename);
free_bprm:
	kfree(bprm);
}
