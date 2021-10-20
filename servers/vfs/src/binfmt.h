/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SERVERS_VFS_SRC_BINFMT_H_
#define __SERVERS_VFS_SRC_BINFMT_H_

#include <base/list.h>

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

struct minix_rt_binprm {
	char buf[BINPRM_BUF_SIZE];

	const char *filename;
	const void *file;
	unsigned long file_size;

	int argc, envc;
	const char *const *argv;
	const char *const *envp;
};

struct minix_rt_binfmt {
	struct list_head lh;
	int (*load_binary)(struct minix_rt_binprm *);
} __randomize_layout;

extern void __register_binfmt(struct minix_rt_binfmt *fmt, int insert);

/* Registration of default binfmt handlers */
static inline void register_binfmt(struct minix_rt_binfmt *fmt)
{
	__register_binfmt(fmt, 0);
}
/* Same as above, but adds a new binfmt at the top of the list */
static inline void insert_binfmt(struct minix_rt_binfmt *fmt)
{
	__register_binfmt(fmt, 1);
}

extern void unregister_binfmt(struct minix_rt_binfmt *);

/*
 * register elf binfmt
 */
extern int init_elf_binfmt(void);
extern void exit_elf_binfmt(void);

#endif /* !__SERVERS_VFS_SRC_BINFMT_H_ */
