#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <libminix_rt/ipc.h>

#define MAX_INIT_ARGS	32
#define MAX_INIT_ENVS	32

static const char *argv_init[MAX_INIT_ARGS + 2] = { "init", NULL, };
static const char *envp_init[MAX_INIT_ENVS + 2] = { "HOME=/", "TERM=minix_rt", NULL, };

static int do_execve(const char *filename, const char *const argv[],
					int argv_len, const char *const envp[], int envp_len)
{
	int ret;
	message_t m;

	memset(&m, 0, sizeof (message_t));
	m.m_type = IPC_M_TYPE_VFS_EXEC;
	m.m_vfs_exec.filename = filename;
	m.m_vfs_exec.filename_len = strlen(filename) + 1;
	m.m_vfs_exec.argv = argv;
	m.m_vfs_exec.argv_len = argv_len;
	m.m_vfs_exec.envp = envp;
	m.m_vfs_exec.envp_len = envp_len;

	ret = ipc_send(ENDPOINT_VFS, &m);
	if (ret)
		panic("Init process send message fail! %d\n", ret);

	return m.m_vfs_exec.retval;
}

static int run_init_process(const char *init_filenmae)
{
	argv_init[0] = init_filenmae;
	printf("Run %s as init process\n", init_filenmae);
	return do_execve(init_filenmae,
				(const char *const *)argv_init, ARRAY_SIZE(argv_init),
				(const char *const *)envp_init, ARRAY_SIZE(envp_init));
}

static int try_to_run_init_process(const char *init_filename)
{
	int ret;

	ret = run_init_process(init_filename);
	if (ret && ret != -ENOENT)
		printf("Starting init: %s exists but couldn't execute it (error %d)\n",
				init_filename, ret);

	return ret;
}

int main(void)
{
	try_to_run_init_process("/sbin/init");
	try_to_run_init_process("/etc/init");
	try_to_run_init_process("/bin/init");
	try_to_run_init_process("/bin/sh");

	panic("No working init found.  Try passing init= option to kernel.\n");

	return 0;
}
