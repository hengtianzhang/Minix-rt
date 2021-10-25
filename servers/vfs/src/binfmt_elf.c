#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>

#include <base/elf.h>
#include <base/auxvec.h>

#include <asm/base/page-def.h>

#include <libminix_rt/exec.h>
#include <libminix_rt/ipc.h>
#include <libminix_rt/mmap.h>

#include "binfmt.h"

/* That's for binfmt_elf_fdpic to deal with */
#ifndef elf_check_fdpic
#define elf_check_fdpic(ex) false
#endif

#if ELF_EXEC_PAGESIZE > PAGE_SIZE
#define ELF_MIN_ALIGN	ELF_EXEC_PAGESIZE
#else
#define ELF_MIN_ALIGN	PAGE_SIZE
#endif

#define ELF_PAGESTART(_v) ((_v) & ~(unsigned long)(ELF_MIN_ALIGN-1))
#define ELF_PAGEOFFSET(_v) ((_v) & (ELF_MIN_ALIGN-1))
#define ELF_PAGEALIGN(_v) (((_v) + ELF_MIN_ALIGN - 1) & ~(ELF_MIN_ALIGN - 1))

#define BAD_ADDR(x) ((unsigned long)(x) >= task_size)

#ifndef ELF_BASE_PLATFORM
/*
 * AT_BASE_PLATFORM indicates the "real" hardware/microarchitecture.
 * If the arch defines ELF_BASE_PLATFORM (in asm/elf.h), the value
 * will be copied to the user stack in the same manner as AT_PLATFORM.
 */
#define ELF_BASE_PLATFORM NULL
#endif

#define STACK_ADD(sp, items) ((elf_addr_t __user *)(sp) - (items))
#define STACK_ROUND(sp, items) \
	(((unsigned long) (sp - items)) &~ 15UL)
#define STACK_ALLOC(sp, len) ({ sp -= len ; sp; })

static struct elf_phdr *load_elf_phdrs(struct elfhdr *elf_ex,
				       const void *elf_file)
{
	struct elf_phdr *elf_phdata = NULL;
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

	elf_phdata = malloc(size);
	if (!elf_phdata)
		goto out;

	memcpy(elf_phdata, elf_file + pos, size);

out:
	return elf_phdata;
}

static int elf_map(struct minix_rt_binprm *bprm, const void *file, unsigned long addr,
			struct elf_phdr *eppnt, int prot)
{
	struct mmap_binprm_info *binprm_info;
	unsigned long size = eppnt->p_memsz + ELF_PAGEOFFSET(eppnt->p_vaddr);
	unsigned long off = eppnt->p_offset - ELF_PAGEOFFSET(eppnt->p_vaddr);
	addr = ELF_PAGESTART(addr);
	size = ELF_PAGEALIGN(size);

	if (!size)
		return -EFAULT;

	binprm_info = malloc(sizeof (struct mmap_binprm_info));
	if (!binprm_info)
		return -ENOMEM;

	binprm_info->vaddr = addr;
	binprm_info->size = size;
	binprm_info->off = off;
	binprm_info->prot = prot;
	binprm_info->next = bprm->binprm_info;
	bprm->binprm_info = binprm_info;

	return 0;
}

static int
create_elf_tables(struct minix_rt_binprm *bprm, unsigned long task_size)
{
	unsigned long top = task_size;
	const char *k_platform = ELF_PLATFORM;
	const char *k_base_platform = ELF_BASE_PLATFORM;
	elf_addr_t *u_platform;
	elf_addr_t *u_base_platform;
	elf_addr_t *u_rand_bytes;
	unsigned long u_envp = 0, u_argv = 0;
	unsigned char k_rand_bytes[16];
	elf_addr_t *elf_info;
	int cnt, ret, i;
	int ei_index = 0;
	int items, argc, envc;
	elf_addr_t *addr;
	size_t len;

	u_platform = NULL;
	if (k_platform) {
		len = strlen(k_platform) + 1;
		u_platform = (elf_addr_t *)STACK_ALLOC(top, len);
	}

	u_base_platform = NULL;
	if (k_base_platform) {
		len = strlen(k_base_platform) + 1;
		u_base_platform = (elf_addr_t *)STACK_ALLOC(top, len);
	}

	get_random_bytes(k_rand_bytes, sizeof (k_rand_bytes));
	u_rand_bytes = (elf_addr_t *)
		       STACK_ALLOC(top, sizeof(k_rand_bytes));

	STACK_ALLOC(top, sizeof (void *));

	bprm->env_end = top;
	for (i = bprm->envc - 1; i >= 0; i--) {
		u_envp = (elf_addr_t)
		STACK_ALLOC(top, ALIGN(strlen((const char *)bprm->envps[i]) + 1, sizeof (void *)));
	}
	bprm->env_start = (unsigned long)u_envp;

	bprm->arg_end = top;
	for (i = bprm->argc - 1; i >= 0; i--) {
		u_argv = (elf_addr_t)
		STACK_ALLOC(top, ALIGN(strlen((const char *)bprm->argvs[i]) + 1, sizeof (void *)));
	}
	u_argv = (elf_addr_t)
		STACK_ALLOC(top, ALIGN(strlen(bprm->filename) + 1, sizeof (void *)));
	bprm->arg_start = (unsigned long)u_argv;

	elf_info = (elf_addr_t *)bprm->saved_auxv;
	/* update AT_VECTOR_SIZE_BASE if the number of NEW_AUX_ENT() changes */
#define NEW_AUX_ENT(id, val) \
	do { \
		elf_info[ei_index++] = id; \
		elf_info[ei_index++] = val; \
	} while (0)

	cnt = get_arch_auxvec_cnt();
	if (cnt) {
		for (i = 0; i < cnt; i++) {
			ret = get_arch_auxvec(&elf_info[ei_index], i);
			if (ret)
				return ret;
			ei_index += 2;
		}
	}

	NEW_AUX_ENT(AT_HWCAP, get_arch_elf_hwcap());
	NEW_AUX_ENT(AT_PAGESZ, ELF_EXEC_PAGESIZE);
	NEW_AUX_ENT(AT_CLKTCK, 100);
	/* TODO dync lib, uselib */
	//NEW_AUX_ENT(AT_PHDR, 0 + exec->e_phoff);
	NEW_AUX_ENT(AT_PHENT, sizeof(struct elf_phdr));
	//NEW_AUX_ENT(AT_PHNUM, exec->e_phnum);
	//NEW_AUX_ENT(AT_BASE, interp_load_addr);
	NEW_AUX_ENT(AT_FLAGS, 0);
	//NEW_AUX_ENT(AT_ENTRY, exec->e_entry);
	NEW_AUX_ENT(AT_RANDOM, (elf_addr_t)(unsigned long)u_rand_bytes);
	if (k_platform) {
		NEW_AUX_ENT(AT_PLATFORM,
			    (elf_addr_t)(unsigned long)u_platform);
	}
	if (k_base_platform) {
		NEW_AUX_ENT(AT_BASE_PLATFORM,
			    (elf_addr_t)(unsigned long)u_base_platform);
	}
#undef NEW_AUX_ENT
	/* AT_NULL is zero; clear the rest too */
	memset(&elf_info[ei_index], 0,
	       sizeof (bprm->saved_auxv) - ei_index * sizeof elf_info[0]);
	/* And advance past the AT_NULL entry.  */
	ei_index += 2;
	top = (unsigned long)STACK_ADD(top, ei_index);

	items = (bprm->argc + 1) + (bprm->envc + 1) + 1 + 1;
	top = (unsigned long)STACK_ADD(top, items);

	addr = (elf_addr_t *)malloc(task_size - top);
	if (!addr)
		return -ENOMEM;

	bprm->malloc_p = (unsigned long)addr;
	bprm->p = top;

	*addr++ = bprm->argc + 1;
	top += sizeof (elf_addr_t);
	argc = bprm->argc;

	*addr++ = u_argv;
	top += sizeof (elf_addr_t);

	u_argv += ALIGN(strlen(bprm->filename) + 1, sizeof (void *));
	for (i = 0; i < bprm->argc; i++) {
		*addr = u_argv;
		u_argv += ALIGN(strlen((const char *)bprm->argvs[i]) + 1, sizeof (void *));
		addr++;
		top += sizeof (elf_addr_t);
	}
	*addr++ = 0;
	top += sizeof (elf_addr_t);

	envc = bprm->envc;
	for (i = 0; i < bprm->envc; i++) {
		*addr = u_envp;
		u_envp += ALIGN(strlen((const char *)bprm->envps[i]) + 1, sizeof (void *));
		addr++;
		top += sizeof (elf_addr_t);
	}
	*addr++ = 0;
	top += sizeof (elf_addr_t);

	memcpy(addr, elf_info, ei_index * sizeof (elf_addr_t));
	addr += ei_index;
	top += ei_index * sizeof (elf_addr_t);

	memcpy(addr, bprm->filename, strlen(bprm->filename) + 1);
	addr = (void *)((unsigned long)addr + ALIGN(strlen(bprm->filename) + 1, sizeof (void *)));
	top += ALIGN(strlen(bprm->filename) + 1, sizeof (void *));

	argc = bprm->argc;
	for (i = 0; i < argc; i++) {
		memcpy(addr, bprm->argvs[i], strlen((const char *)bprm->argvs[i]) + 1);
		addr = (void *)((unsigned long)addr + ALIGN(strlen((const char *)bprm->argvs[i]) + 1, sizeof (void *)));
		top += ALIGN(strlen((const char *)bprm->argvs[i]) + 1, sizeof (void *));
	}

	envc = bprm->envc;
	for (i = 0; i < envc; i++) {
		memcpy(addr, bprm->envps[i], strlen((const char *)bprm->envps[i]) + 1);
		addr = (void *)((unsigned long)addr + ALIGN(strlen((const char *)bprm->envps[i]) + 1, sizeof (void *)));
		top += ALIGN(strlen((const char *)bprm->envps[i]) + 1, sizeof (void *));

	}
	*addr++ = 0;
	top += sizeof (elf_addr_t);

	memcpy(addr, k_rand_bytes, sizeof (k_rand_bytes));
	addr = (void *)((unsigned long)addr + sizeof (k_rand_bytes));
	top += sizeof (k_rand_bytes);

	if (k_base_platform) {
		memcpy(addr, k_base_platform, strlen(k_base_platform) + 1);
		addr = (void *)((unsigned long)addr + ALIGN(strlen(k_base_platform) + 1, sizeof (void *)));
		top += ALIGN(strlen(k_base_platform) + 1, sizeof (void *));
	}

	if (k_platform) {
		memcpy(addr, k_platform, strlen(k_platform) + 1);
		addr = (void *)((unsigned long)addr + ALIGN(strlen(k_platform) + 1, sizeof (void *)));
		top += ALIGN(strlen(k_platform) + 1, sizeof (void *));
	}

	return 0;
}

/**
 * load_elf_phdrs() - load ELF program headers
 * @elf_ex:   ELF header of the binary whose program headers should be loaded
 * @elf_file: the opened ELF binary file
 *
 * Loads ELF program headers from the binary file elf_file, which has the ELF
 * header pointed to by elf_ex, into a newly allocated array. The caller is
 * responsible for freeing the allocated data. Returns an ERR_PTR upon failure.
 */
static int load_elf_binary(struct minix_rt_binprm *bprm)
{
	int retval, i;
	int load_addr_set = 0;
	unsigned long task_size;
	unsigned long load_addr __maybe_unused = 0;
	struct mmap_binprm_info *next, *tmp_next;
	struct elf_phdr *elf_ppnt, *elf_phdata;
	unsigned long elf_bss, elf_brk;
	int bss_prot __maybe_unused = 0;
	unsigned long elf_entry;
	unsigned long start_code, end_code, start_data, end_data;
	struct {
		struct elfhdr elf_ex;
		struct elfhdr interp_elf_ex;
	} *loc;

	retval = -EIO;
	task_size = get_task_size();
	if (!task_size)
		goto out_ret;
	
	loc = malloc(sizeof (*loc));
	if (!loc) {
		retval = -ENOMEM;
		goto out_ret;
	}

	/* Get the exec-header */
	loc->elf_ex = *((struct elfhdr *)bprm->buf);

	retval = -ENOEXEC;
	/* First of all, some simple consistency checks */
	if (memcmp(loc->elf_ex.e_ident, ELFMAG, SELFMAG) != 0)
		goto free_loc;

	if (loc->elf_ex.e_type != ET_EXEC)
		goto free_loc;
	if (!elf_check_arch(&loc->elf_ex))
		goto free_loc;
	if (elf_check_fdpic(&loc->elf_ex))
		goto free_loc;

	elf_phdata = load_elf_phdrs(&loc->elf_ex, bprm->file);
	if (!elf_phdata)
		goto free_loc;

	elf_bss = 0;
	elf_brk = 0;

	start_code = ~0UL;
	end_code = 0;
	start_data = 0;
	end_data = 0;

	/* Now we do a little grungy work by mmapping the ELF image into
	   the correct location in memory. */
	for(i = 0, elf_ppnt = elf_phdata;
	    i < loc->elf_ex.e_phnum; i++, elf_ppnt++) {
		int elf_prot = 0;
		unsigned long k, vaddr;

		if (elf_ppnt->p_type != PT_LOAD)
			continue;

		if (unlikely (elf_brk > elf_bss)) {
			printf("TODO elf_brk 0x%lx > elf_bss 0x%lx\n", elf_brk, elf_bss);
		}

		if (elf_ppnt->p_flags & PF_R)
			elf_prot |= VM_READ;
		if (elf_ppnt->p_flags & PF_W)
			elf_prot |= VM_WRITE;
		if (elf_ppnt->p_flags & PF_X)
			elf_prot |= VM_EXEC;

		vaddr = elf_ppnt->p_vaddr;

		retval = elf_map(bprm, bprm->file, vaddr, elf_ppnt,
						elf_prot);
		if (retval) {
			goto free_elf_phdata;
		}

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
		    elf_ppnt->p_memsz > task_size ||
		    task_size - elf_ppnt->p_memsz < k) {
			/* set_brk can never work. Avoid overflows. */
			retval = -EINVAL;
			goto free_elf_phdata;
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

	elf_entry = loc->elf_ex.e_entry;
	if (BAD_ADDR(elf_entry)) {
		retval = -EINVAL;
		goto free_elf_phdata;
	}

	bprm->e_entry = elf_entry;
	bprm->start_code = start_code;
	bprm->end_code = end_code;
	bprm->start_data = start_data;
	bprm->end_data = end_data;
	bprm->bss = elf_bss;
	bprm->brk = elf_brk;

	retval = create_elf_tables(bprm, task_size);
	if (retval)
		goto free_elf_phdata;

	retval = execve(bprm);

	free((void *)bprm->malloc_p);
free_elf_phdata:
	next = bprm->binprm_info;
	while (next) {
		tmp_next = next->next;
		free(next);
		next = tmp_next;
	}
	free(elf_phdata);
free_loc:
	free(loc);
out_ret:
    return retval;
}

static struct minix_rt_binfmt elf_format = {
    .load_binary = load_elf_binary,
};

int init_elf_binfmt(void)
{
	register_binfmt(&elf_format);
	return 0;
}

void exit_elf_binfmt(void)
{
	/* Remove the COFF and ELF loaders. */
	unregister_binfmt(&elf_format);
}
