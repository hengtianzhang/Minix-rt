/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SERVERS_VFS_SRC_BINFMT_H_
#define __SERVERS_VFS_SRC_BINFMT_H_

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

	int argc, envc;
};

#endif /* !__SERVERS_VFS_SRC_BINFMT_H_ */
