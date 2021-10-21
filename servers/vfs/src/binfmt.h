/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __SERVERS_VFS_SRC_BINFMT_H_
#define __SERVERS_VFS_SRC_BINFMT_H_

#include <base/list.h>

#include <minix_rt/binfmt.h>

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
