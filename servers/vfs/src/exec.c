#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <libminix_rt/ipc.h>

#include <base/list.h>

#include <cpio/cpio.h>

#include "exec.h"
#include "fs.h"
#include "binfmt.h"

static LIST_HEAD(formats);

void __register_binfmt(struct minix_rt_binfmt *fmt, int insert)
{
	BUG_ON(!fmt);
	if (WARN_ON(!fmt->load_binary))
		return;
	insert ? list_add(&fmt->lh, &formats) :
		 list_add_tail(&fmt->lh, &formats);
}

void unregister_binfmt(struct minix_rt_binfmt *fmt)
{
	list_del(&fmt->lh);
}

static inline int search_binary_handler(struct minix_rt_binprm *bprm)
{
	struct minix_rt_binfmt *fmt;
	int retval;

	retval = -ENOENT;
	list_for_each_entry(fmt, &formats, lh) {
		retval = fmt->load_binary(bprm);
		if (!retval)
			break;
	}
	return retval;
}

static int exec_binprm(struct minix_rt_binprm *bprm)
{
	return search_binary_handler(bprm);
}

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
static int __count(struct user_arg_ptr argv, int max, pid_t pid,
					struct minix_rt_binprm *bprm, int flags)
{
	int ret, i, j;
	const char **native;

	native = malloc(argv.len);
	if (!native)
		return -ENOMEM;

	ret = message_memcpy(native, argv.ptr.native, argv.len, pid);
	if (ret != argv.len) {
		ret = -EINVAL;
		goto free;
	}

	if (flags) {
		bprm->argvs = malloc(sizeof (void *) * argv.len);
		if (!bprm->argvs) {
			ret = -ENOMEM;
			goto free;
		}
	} else {
		bprm->envps = malloc(sizeof (void *) * argv.len);
		if (!bprm->envps) {
			ret = -ENOMEM;
			goto free;
		}
	}

	i = 0;
	for (;;) {
		const char *p = native[i];
		size_t len;

		if (!p)
			break;

		if (i >= max) {
			ret = -E2BIG;
			goto free_str;
		}

		ret = message_strlen(p, pid);
		if (ret <= 0)
			goto free_str;

		if (flags) {
			bprm->argvs[i] = malloc(ret);
			if (!bprm->argvs[i]) {
				ret = -ENOMEM;
				goto free_str;
			}

			len = message_memcpy(bprm->argvs[i], p, ret, pid);
			if (ret != len) {
				ret = -EINVAL;
				++i;
				goto free_str;
			}
		} else {
			bprm->envps[i] = malloc(ret);
			if (!bprm->envps[i]) {
				ret = -ENOMEM;
				goto free_str;
			}

			len = message_memcpy(bprm->envps[i], p, ret, pid);
			if (ret != len) {
				ret = -EINVAL;
				++i;
				goto free_str;
			}	
		}

		bprm->p += len;
		++i;
	}

	ret = i;

free:
	free(native);
	return ret;

free_str:
	if (flags) {
		for (j = 0; j < i; j++) {
			free(bprm->argvs[j]);
		}
		free(bprm->argvs);
	} else {
		for (j = 0; j < i; j++) {
			free(bprm->envps[j]);
		}
		free(bprm->envps);
	}

	goto free;
}

/*
 * count() counts the number of strings in array ARGV.
 */
static int arg_count(struct user_arg_ptr argv, int max, pid_t pid, struct minix_rt_binprm *bprm)
{
	return __count(argv, max, pid, bprm, 1);
}

/*
 * count() counts the number of strings in array ARGV.
 */
static int env_count(struct user_arg_ptr argv, int max, pid_t pid, struct minix_rt_binprm *bprm)
{
	return __count(argv, max, pid, bprm, 0);
}

static void free_count(struct minix_rt_binprm *bprm)
{
	int i;

	for (i = 0; i < bprm->argc; i++)
		free(bprm->argvs[i]);
	free(bprm->argvs);

	for (i = 0; i < bprm->envc; i++)
		free(bprm->envps[i]);
	free(bprm->envps);
}

static int prepare_arg_pages(struct minix_rt_binprm *bprm, message_t *m)
{
	struct user_arg_ptr argv;

	argv.ptr.native = m->m_vfs_exec.argv;
	argv.len = m->m_vfs_exec.argv_len;
	bprm->argc = arg_count(argv, MAX_ARG_STRINGS, m->m_source, bprm);
	if (bprm->argc < 0)
		return bprm->argc;

	argv.ptr.native = m->m_vfs_exec.envp;
	argv.len = m->m_vfs_exec.envp_len;
	bprm->envc = env_count(argv, MAX_ARG_STRINGS, m->m_source, bprm);
	if (bprm->envc < 0)
		return bprm->argc;

	return 0;
}

static int prepare_binprm(struct minix_rt_binprm *bprm)
{
	memset(bprm->buf, 0, BINPRM_BUF_SIZE);
	memcpy(bprm->buf, bprm->file, BINPRM_BUF_SIZE);

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

	bprm->p = 0;
	retval = prepare_arg_pages(bprm, m);
	if (retval < 0)
		goto out_free;

	bprm->pid = m->m_source;
	bprm->file = filename->file;
	bprm->file_size = filename->file_size;
	bprm->filename = filename->name;
	bprm->filename_size = strlen(filename->name) + 1;
	bprm->argv = m->m_vfs_exec.argv;
	bprm->envp = m->m_vfs_exec.envp;
	bprm->binprm_info = NULL;

	retval = prepare_binprm(bprm);
	if (retval < 0)
		goto out_free;

	retval = exec_binprm(bprm);

	free_count(bprm);
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
	int ret;
	struct filename *filename;
	struct cpio_info info;
	const void *file;
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
#if 0
	for (i = 0; i < info.file_count; i++) {
		file = cpio_get_entry((const void *)initrd_start, initrd_size,
						i, &file_name, &file_size);
		if (strcmp(file_name, name) == 0) {
			filename->file = file;
			filename->file_size = file_size;
			break;
		}
	}
#endif
/*
 * Temp all process to /bin/busybox
 */
	file = cpio_get_file((const void *)initrd_start, initrd_size, "bin/busybox", &file_size);
	if (file) {
		filename->file = file;
		filename->file_size = file_size;
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
