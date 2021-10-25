#ifndef __UAPI_MINIX_RT_BINFMT_H_
#define __UAPI_MINIX_RT_BINFMT_H_

#include <base/auxvec.h>

#include <asm/base/page-def.h>

/* Stack area protections */
#define EXSTACK_DEFAULT   0	/* Whatever the arch defaults to */
#define EXSTACK_DISABLE_X 1	/* Disable executable stacks */
#define EXSTACK_ENABLE_X  2	/* Enable executable stacks */

/*
 * These are the maximum length and maximum number of strings passed to the
 * execve() system call.  MAX_ARG_STRLEN is essentially random but serves to
 * prevent the kernel from being unduly impacted by misaddressed pointers.
 * MAX_ARG_STRINGS is chosen to fit in a signed 32-bit integer.
 */
#define MAX_ARG_STRLEN (PAGE_SIZE * 32)
#define MAX_ARG_STRINGS 0x7FFFFFFF

/* sizeof(linux_binprm->buf) */
#define BINPRM_BUF_SIZE 128

struct mmap_binprm_info {
	unsigned long vaddr;
	unsigned long size;
	unsigned long off;
	int prot;
	struct mmap_binprm_info *next;
};

struct minix_rt_binprm {
	char buf[BINPRM_BUF_SIZE];

	const char *filename;
	unsigned long filename_size;
	const void *file;
	unsigned long file_size;

	int argc, envc;
	const char *const *argv;
	const char *const *envp;

	struct mmap_binprm_info *binprm_info;

	unsigned long e_entry;
	unsigned long start_code, end_code, start_data, end_data;
	unsigned long arg_start, arg_end, env_start, env_end;
	unsigned long bss, brk;

	unsigned long saved_auxv[AT_VECTOR_SIZE];

	unsigned long p, malloc_p;
	unsigned long **argvs, **envps;

	pid_t pid;
};

#endif /* !__UAPI_MINIX_RT_BINFMT_H_ */
