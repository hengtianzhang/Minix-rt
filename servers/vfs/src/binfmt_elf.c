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
create_elf_tables(struct minix_rt_binprm *bprm)
{
	unsigned long p = bprm->p;
	const char *k_platform = ELF_PLATFORM;
	const char *k_base_platform = ELF_BASE_PLATFORM;
	unsigned char k_rand_bytes[16];
	elf_addr_t *elf_info;

	if (k_platform) {
		size_t len = strlen(k_platform) + 1;

		p += len;
	}
	if (k_base_platform) {
		size_t len = strlen(k_base_platform) + 1;
		p += len;
	}


	get_random_bytes(k_rand_bytes, sizeof (k_rand_bytes));
	p += sizeof (k_rand_bytes);

	elf_info = (elf_addr_t *)bprm->saved_auxv;
	/* update AT_VECTOR_SIZE_BASE if the number of NEW_AUX_ENT() changes */
#define NEW_AUX_ENT(id, val) \
	do { \
		elf_info[ei_index++] = id; \
		elf_info[ei_index++] = val; \
	} while (0)

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

	retval = create_elf_tables(bprm);
	if (retval)
		goto free_elf_phdata;

	retval = execve(bprm);
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
