#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libminix_rt/ipc.h>

#include <cpio/cpio.h>

#include "exec.h"
#include "fs.h"
#include "binfmt.h"

unsigned long initrd_start = 0;
unsigned long initrd_size = 0;

struct user_arg_ptr {
	union {
		const char *const *native;
	} ptr;
	int len;
};

/*
 * count() counts the number of strings in array ARGV.
 */
static int count(struct user_arg_ptr argv, int max, pid_t pid)
{
	int ret, i;
	const char **native;

	native = malloc(argv.len);
	if (!native)
		return -ENOMEM;

	ret = message_memcpy(native, argv.ptr.native, argv.len, pid);
	if (ret != argv.len)
		return -EINVAL;

	i = 0;
	for (;;) {
		const char *p = native[i];

		if (!p)
			break;

		if (i >= max)
			return -E2BIG;
		++i;
	}

	return i;
}

static int prepare_arg_pages(struct minix_rt_binprm *bprm, message_t *m)
{
	struct user_arg_ptr argv;

	argv.ptr.native = m->m_vfs_exec.argv;
	argv.len = m->m_vfs_exec.argv_len;
	bprm->argc = count(argv, MAX_ARG_STRINGS, m->m_source);
	if (bprm->argc < 0)
		return bprm->argc;

	argv.ptr.native = m->m_vfs_exec.envp;
	argv.len = m->m_vfs_exec.envp_len;
	bprm->envc = count(argv, MAX_ARG_STRINGS, m->m_source);
	if (bprm->envc < 0)
		return bprm->argc;

	return 0;
}

static int do_execve_file(struct filename *filename, message_t *m)
{
	int retval;
	struct minix_rt_binprm *bprm;

	if (IS_ERR(filename))
		return PTR_ERR(filename);

	retval = -ENOMEM;
	bprm = zalloc(sizeof (struct minix_rt_binprm));
	if (!bprm)
		goto out;

	retval = prepare_arg_pages(bprm, m);
	if (retval < 0)
		goto out_free;

	retval = 0;
out_free:
	free(bprm);
out:
	free(filename);

	return retval;
}

int do_execve(struct filename *filename, message_t *m)
{

	return do_execve_file(filename, m);
}

struct filename *get_filename(const char *name)
{
	int ret, i;
	struct filename *filename;
	struct cpio_info info;
	const void *file;
	const char *file_name;
	unsigned long file_size;

	if (!name)
		return ERR_PTR(-EINVAL);

	filename = malloc(sizeof (struct filename));
	if (!filename) {
		return ERR_PTR(-ENOMEM);
	}

	filename->name = name;
	filename->file = NULL;

	if (*name == '/')
		name++;

	ret = cpio_info((const void *)initrd_start, initrd_size, &info);
	if (ret) {
		printf("Get cpio info failed! retval (%d)\n", ret);
		goto free_filename;
	}

	for (i = 0; i < info.file_count; i++) {
		file = cpio_get_entry((const void *)initrd_start, initrd_size,
						i, &file_name, &file_size);
		if (strcmp(file_name, name) == 0) {
			filename->file = file;
			filename->file_size = file_size;
			break;
		}
	}

	if (!filename->file)
		goto free_filename;

	return filename;

free_filename:
	free(filename);

	return ERR_PTR(-ENOENT);
}

void do_exec(endpoint_t ep, message_t *m)
{
	int ret;
	void *filename;

	filename = malloc(m->m_vfs_exec.filename_len);
	if (!filename) {
		printf("do_exec malloc failed!\n");
		m->m_vfs_exec.retval = -ENOMEM;
		return;
	}

	ret = message_memcpy(filename, m->m_vfs_exec.filename,
				m->m_vfs_exec.filename_len, m->m_source);
	if (ret != m->m_vfs_exec.filename_len) {
		printf("message memcpy failed! retval (%d)\n", ret);
		m->m_vfs_exec.retval = ret;
		goto free_filename;
	}

	ret = do_execve(get_filename(filename), m);
	m->m_vfs_exec.retval = ret;

free_filename:
	free(filename);
}
