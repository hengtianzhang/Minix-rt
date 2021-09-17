#include <base/init.h>
#include <base/common.h>
#include <base/elf.h>
#include <base/string.h>
#include <base/errno.h>

#include <sel4m/slab.h>
#include <sel4m/gfp.h>
#include <sel4m/page.h>
#include <sel4m/sched.h>
#include <sel4m/object/tcb.h>
#include <sel4m/object/untype.h>

#include <asm/processor.h>

extern char __start_archive[];
extern char __end_archive[];

#define INIT_SERVICE_COMM "rootService"

#if ELF_EXEC_PAGESIZE > PAGE_SIZE
#define ELF_MIN_ALIGN	ELF_EXEC_PAGESIZE
#else
#define ELF_MIN_ALIGN	PAGE_SIZE
#endif

#define ELF_PAGESTART(_v) ((_v) & ~(unsigned long)(ELF_MIN_ALIGN-1))
#define ELF_PAGEOFFSET(_v) ((_v) & (ELF_MIN_ALIGN-1))
#define ELF_PAGEALIGN(_v) (((_v) + ELF_MIN_ALIGN - 1) & ~(ELF_MIN_ALIGN - 1))

#define BAD_ADDR(x) ((unsigned long)(x) >= TASK_SIZE)

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

static __init int setup_services_stack(struct task_struct *tsk)
{
	int ret;
	unsigned long vm_flags = 0;
	struct vm_area_struct *vma;
	struct pt_regs *regs = task_pt_regs(tsk);
	unsigned long stack_top = STACK_TOP;
	unsigned long stack_base;

	stack_top = PAGE_ALIGN(stack_top);
	stack_base = PAGE_ALIGN(stack_top - THREAD_SIZE);

	vm_flags |= VM_READ | VM_WRITE | VM_EXEC;
	vma = untype_get_vmap_area(stack_base, THREAD_SIZE,
							vm_flags, tsk->mm, 0);
	if (!vma)
		goto fail_vma;

	ret = vmap_page_range(vma);
	if (ret <= 0)
		goto fail_map_vma;

	regs->sp = stack_base;

	return 0;

fail_map_vma:
	untype_free_vmap_area(stack_base, tsk->mm);

fail_vma:
	return -ENOMEM;
}

static int __init set_brk(unsigned long start, unsigned long end, int prot)
{
	start = ELF_PAGEALIGN(start);
	end = ELF_PAGEALIGN(end);
	printf("start 0x%lx end 0x%lx\n", start, end);

	return 0;
}

static __init unsigned long elf_map(unsigned long addr,
		struct elf_phdr *eppnt, int prot)
{
	unsigned long map_addr;
	unsigned long size = eppnt->p_filesz + ELF_PAGEOFFSET(eppnt->p_vaddr);
	unsigned long off = eppnt->p_offset - ELF_PAGEOFFSET(eppnt->p_vaddr);
	addr = ELF_PAGESTART(addr);
	size = ELF_PAGEALIGN(size);

	/* mmap() will return -EINVAL if given a zero size, but a
	 * segment with zero filesize is perfectly valid */
	if (!size)
		return addr;

	printf("sss addr 0x%lx size 0x%lx off 0x%lx\n", addr, size,  off);
}

int __init service_core_init(void)
{
	struct task_struct *tsk;
	int i, ret;
	unsigned long load_addr = 0, load_bias = 0;
	int load_addr_set = 0;
	struct elf_phdr *elf_ppnt, *elf_phdata;
	unsigned long elf_bss, elf_brk;
	int bss_prot = 0;
	unsigned long error;
	unsigned long start_code, end_code, start_data, end_data;
	struct {
		struct elfhdr elf_ex;
	} *loc;

	loc = (void *)__start_archive;

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

	tsk = tcb_create_task();
	if (!tsk)
		goto out;

	strlcpy(tsk->comm, INIT_SERVICE_COMM, sizeof (tsk->comm));

	tsk->stack = kmalloc(THREAD_SIZE, GFP_KERNEL | GFP_ZERO);
	if (!tsk->stack)
		goto fail_stack;

	ret = setup_services_stack(tsk);
	if (ret)
		goto fail_service_stack;

	/* Now we do a little grungy work by mmapping the ELF image into
	   the correct location in memory. */
	for(i = 0, elf_ppnt = elf_phdata;
	    i < loc->elf_ex.e_phnum; i++, elf_ppnt++) {
		int elf_prot = 0, elf_flags;
		unsigned long k, vaddr;
		unsigned long total_size = 0;

		if (elf_ppnt->p_type != PT_LOAD)
			continue;

		if (unlikely (elf_brk > elf_bss)) {
			unsigned long nbyte;

			ret = set_brk(elf_bss + load_bias,
					elf_brk + load_bias,
					bss_prot);
			if (ret)
				goto fail_service_stack;
			nbyte = ELF_PAGEOFFSET(elf_bss);
			if (nbyte) {
				nbyte = ELF_MIN_ALIGN - nbyte;
				if (nbyte > elf_brk - elf_bss)
					nbyte = elf_brk - elf_bss;
				printf("elf_bss 0x%lx nbyte 0x%lx\n", elf_bss, nbyte);
				//if (clear_user((void __user *)elf_bss +
				//			load_bias, nbyte)) {
				//	/*
				//	 * This bss-zeroing can fail if the ELF
				//	 * file specifies odd protections. So
				//	 * we don't check the return value
				//	 */
				//}
			}
		}

		if (elf_ppnt->p_flags & PF_R)
			elf_prot |= VM_READ;
		if (elf_ppnt->p_flags & PF_W)
			elf_prot |= VM_WRITE;
		if (elf_ppnt->p_flags & PF_X)
			elf_prot |= VM_EXEC;

		vaddr = elf_ppnt->p_vaddr;

		error = elf_map(load_bias + vaddr, elf_ppnt, elf_prot);


		printf("elf_prot 0x%lx 0x%lx\n", elf_prot,  loc->elf_ex.e_entry);


	}





	return 0;

fail_service_stack:
	kfree(tsk->stack);
fail_stack:
	tcb_destroy_task(tsk);

out:
	hang("The core service cannot be initialized!\n");

	return -ENOENT;
}
