#include <base/init.h>
#include <base/common.h>
#include <base/elf.h>
#include <base/string.h>
#include <base/errno.h>

#include <cpio/cpio.h>

#include <minix_rt/cpumask.h>
#include <minix_rt/slab.h>
#include <minix_rt/gfp.h>
#include <minix_rt/page.h>
#include <minix_rt/sched.h>
#include <minix_rt/sched/rt.h>
#include <minix_rt/mmap.h>
#include <minix_rt/uaccess.h>

#include <asm/processor.h>

extern char __start_archive[];
extern char __end_archive[];

extern char __start_archive_drivers[];
extern char __end_archive_drivers[];

#define INIT_SERVICE_COMM "rootService"

#if ELF_EXEC_PAGESIZE > PAGE_SIZE
#define ELF_MIN_ALIGN	ELF_EXEC_PAGESIZE
#else
#define ELF_MIN_ALIGN	PAGE_SIZE
#endif

#define ELF_PAGESTART(_v) ((_v) & ~(unsigned long)(ELF_MIN_ALIGN-1))
#define ELF_PAGEOFFSET(_v) ((_v) & (ELF_MIN_ALIGN-1))
#define ELF_PAGEALIGN(_v) (((_v) + ELF_MIN_ALIGN - 1) & ~(ELF_MIN_ALIGN - 1))

#define TYPE_DRIVERS 1
#define TYPE_SERVERS 2

static __init struct elf_phdr *load_elf_phdrs(struct elfhdr *elf_ex)
{
	int size;
	loff_t pos = elf_ex->e_phoff;

	/*
	 * If the size of this structure has changed, then punt, since
	 * we will be doing the wrong thing.
	 */
	if (elf_ex->e_phentsize != sizeof(struct elf_phdr))
		goto out;

	/* Sanity check the number of program headers... */
	if (elf_ex->e_phnum < 1 ||
		elf_ex->e_phnum > 65536U / sizeof(struct elf_phdr))
		goto out;

	/* ...and their total size. */
	size = sizeof(struct elf_phdr) * elf_ex->e_phnum;
	if (size > ELF_MIN_ALIGN)
		goto out;

	return (struct elf_phdr *)((loff_t)elf_ex + pos);

out:
	return NULL;
}

static __init unsigned long setup_services_stack(struct task_struct *tsk, unsigned long *top)
{
	int ret;
	unsigned long vm_flags = 0;
	struct vm_area_struct *vma;
	unsigned long stack_top = STACK_TOP;
	unsigned long stack_base;

	stack_top = PAGE_ALIGN(stack_top);
	stack_base = PAGE_ALIGN(stack_top - THREAD_SIZE);

	vm_flags |= VM_READ | VM_WRITE | VM_USER_STACK;
	vma = mmap_get_vmap_area(stack_base, THREAD_SIZE,
							vm_flags, tsk->mm, 0);
	if (!vma)
		goto fail_vma;

	ret = vmap_page_range(vma);
	if (ret <= 0)
		goto fail_map_vma;

	if (top)
		*top = stack_top;

	return stack_base;

fail_map_vma:
	mmap_free_vmap_area(stack_base, tsk->mm);

fail_vma:
	return 0;
}

static __init unsigned long elf_map(void *archive_base,
		unsigned long addr, struct elf_phdr *eppnt, int prot, struct task_struct *tsk)
{
	int i;
	struct vm_area_struct *vma;
	unsigned long size = eppnt->p_memsz + ELF_PAGEOFFSET(eppnt->p_vaddr);
	unsigned long off = eppnt->p_offset - ELF_PAGEOFFSET(eppnt->p_vaddr);
	addr = ELF_PAGESTART(addr);
	size = ELF_PAGEALIGN(size);

	/* mmap() will return -EINVAL if given a zero size, but a
	 * segment with zero filesize is perfectly valid */
	if (!size)
		return -EFAULT;

	vma = mmap_get_vmap_area(addr, size, prot, tsk->mm, 0);
	if (!vma)
		return -ENOMEM;

	BUG_ON(vma->nr_pages != (size/PAGE_SIZE));

	for (i = 0; i < vma->nr_pages; i++) {
		void *srcaddr = page_to_virt(vma->pages[i]);
		void *dstaddr = archive_base + off + i * PAGE_SIZE;
		memcpy(srcaddr, dstaddr, PAGE_SIZE);
	}
	i = vmap_page_range(vma);
	if (i <= 0) {
		mmap_free_vmap_area(addr, tsk->mm);
		return -ENOMEM;
	}

	if (vma->vm_start < tsk->mm->mmap_base)
		tsk->mm->mmap_base = vma->vm_start;
	if (vma->vm_end > tsk->mm->mmap_end)
		tsk->mm->mmap_end = vma->vm_end;

	return 0;
}

asmlinkage void ret_from_fork(void) asm("ret_from_fork");

__init struct task_struct *service_core_init(int type,
			unsigned long elf_start_archive, const char *name)
{
	cpumask_t mask;
	struct pt_regs *regs;
	struct task_struct *tsk;
	int i, ret;
	unsigned long stack_base = 0, stack_top;
	unsigned long load_addr = 0, load_bias = 0;
	int load_addr_set = 0;
	struct elf_phdr *elf_ppnt, *elf_phdata;
	unsigned long elf_bss, elf_brk;
	int bss_prot = 0;
	unsigned long elf_entry;
	unsigned long start_code, end_code, start_data, end_data;
	struct {
		struct elfhdr elf_ex;
	} *loc;

	loc = (void *)elf_start_archive;

	if (memcmp(loc->elf_ex.e_ident, ELFMAG, SELFMAG) != 0)
		goto out;

	if (loc->elf_ex.e_type != ET_EXEC)
		goto out;
	if (!elf_check_arch(&loc->elf_ex))
		goto out;

	elf_phdata = load_elf_phdrs(&loc->elf_ex);
	if (!elf_phdata)
		goto out;

	elf_bss = 0;
	elf_brk = 0;

	start_code = ~0UL;
	end_code = 0;
	start_data = 0;
	end_data = 0;

	tsk = task_create_tsk(PF_SYSTEMSERVICE);
	if (!tsk)
		goto out;

	strlcpy(tsk->comm, name, sizeof (tsk->comm));

	ret = -ENOMEM;
	tsk->stack = kmalloc(THREAD_SIZE, GFP_KERNEL | GFP_ZERO);
	if (!tsk->stack)
		goto fail_stack;

	task_set_stack_end_magic(tsk);

	stack_base = setup_services_stack(tsk, &stack_top);
	if (!stack_base)
		goto fail_service_stack;

	/* Now we do a little grungy work by mmapping the ELF image into
	   the correct location in memory. */
	for(i = 0, elf_ppnt = elf_phdata;
	    i < loc->elf_ex.e_phnum; i++, elf_ppnt++) {
		int elf_prot = 0;
		unsigned long k, vaddr;

		if (elf_ppnt->p_type != PT_LOAD)
			continue;

		BUG_ON(unlikely(elf_brk > elf_bss));

		if (elf_ppnt->p_flags & PF_R)
			elf_prot |= VM_READ;
		if (elf_ppnt->p_flags & PF_W)
			elf_prot |= VM_WRITE;
		if (elf_ppnt->p_flags & PF_X)
			elf_prot |= VM_EXEC;

		vaddr = elf_ppnt->p_vaddr;

		ret = elf_map(loc, load_bias + vaddr, elf_ppnt, elf_prot, tsk);
		if (ret)
			goto fail_service_stack;

		if (!load_addr_set) {
			load_addr_set = 1;
			load_addr = (elf_ppnt->p_vaddr - elf_ppnt->p_offset);
		}
		k = elf_ppnt->p_vaddr;
		if (k < start_code)
			start_code = k;
		if (start_data < k)
			start_data = k;

		/*
		 * Check to see if the section's size will overflow the
		 * allowed task size. Note that p_filesz must always be
		 * <= p_memsz so it is only necessary to check p_memsz.
		 */
		if (BAD_ADDR(k) || elf_ppnt->p_filesz > elf_ppnt->p_memsz ||
		    elf_ppnt->p_memsz > TASK_SIZE ||
		    TASK_SIZE - elf_ppnt->p_memsz < k) {
			/* set_brk can never work. Avoid overflows. */
			ret = -EINVAL;
			goto fail_service_stack;
		}

		k = elf_ppnt->p_vaddr + elf_ppnt->p_filesz;

		if (k > elf_bss)
			elf_bss = k;
		if ((elf_ppnt->p_flags & PF_X) && end_code < k)
			end_code = k;
		if (end_data < k)
			end_data = k;
		k = elf_ppnt->p_vaddr + elf_ppnt->p_memsz;
		if (k > elf_brk) {
			bss_prot = elf_prot;
			elf_brk = k;
		}
	}

	loc->elf_ex.e_entry += load_bias;
	elf_bss += load_bias;
	elf_brk += load_bias;
	start_code += load_bias;
	end_code += load_bias;
	start_data += load_bias;
	end_data += load_bias;

	tsk->mm->start_code = start_code;
	tsk->mm->end_code = end_code;
	tsk->mm->start_data = start_data;
	tsk->mm->end_data = end_data;
	tsk->mm->elf_bss = elf_bss;
	tsk->mm->elf_brk = elf_brk;
	tsk->mm->start_stack = stack_top;

	elf_entry = loc->elf_ex.e_entry;
	if (BAD_ADDR(elf_entry)) {
		ret = -EINVAL;
		goto fail_service_stack;
	}

	task_thread_info(tsk)->addr_limit = USER_DS;

	tsk->state = TASK_RUNNING;
	ret = pid_alloc_pid(tsk);
	if (ret)
		goto fail_service_stack;

	tsk->policy = SCHED_FIFO;
	if (type == TYPE_DRIVERS) {
		tsk->prio = 1;
		tsk->static_prio = 1;
		tsk->normal_prio = 1;
	} else {
		tsk->prio = 2;
		tsk->static_prio = 2;
		tsk->normal_prio = 2;
	}

	tsk->sched_class = &rt_sched_class;
	tsk->time_slice = RR_TIMESLICE;

	mask = CPU_MASK_ALL;
	set_cpus_allowed(tsk, mask);

	sched_fork(tsk, 0);

	regs = task_pt_regs(tsk);
	start_thread(regs, elf_entry, stack_top);
	regs->regs[0] = 0;

	memset(&tsk->thread.cpu_context, 0, sizeof(struct cpu_context));

	tsk->thread.cpu_context.pc = (unsigned long)ret_from_fork;
	tsk->thread.cpu_context.sp = (unsigned long)regs;

	return tsk;

fail_service_stack:
	kfree(tsk->stack);
fail_stack:
	task_destroy_tsk(tsk);
out:
	hang("The core service cannot be initialized!\n");

	return NULL;
}

void __init services_task_init(void)
{
	struct task_struct *tsk;
	int i;
	const char *name;
	const void *elf_start;
	unsigned long start_archive, size;
	unsigned long len;
	struct cpio_info info;

	len = __end_archive_drivers - __start_archive_drivers;
	start_archive = (unsigned long)__start_archive_drivers;

	i = cpio_info((const void *)start_archive, len, &info);
	if (!i) {
		for (i = 0; i < info.file_count; i++) {
			elf_start = cpio_get_entry((const void *)start_archive, len, i, &name, &size);
			tsk = service_core_init(TYPE_DRIVERS, (unsigned long)elf_start, name);
			BUG_ON(!tsk);
			wake_up_new_task(tsk, 0);
		}
	}

	len = __end_archive - __start_archive;
	start_archive = (unsigned long)__start_archive;
	i = cpio_info((const void *)start_archive, len, &info);
	BUG_ON(i);

	for (i = 0; i < info.file_count; i++) {
		elf_start = cpio_get_entry((const void *)start_archive, len, i, &name, &size);
		tsk = service_core_init(TYPE_SERVERS, (unsigned long)elf_start, name);
		BUG_ON(!tsk);
		wake_up_new_task(tsk, 0);
	}
}
