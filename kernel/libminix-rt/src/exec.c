#include <string.h>
#include <errno.h>

#include <minix_rt/binfmt.h>

#include <libminix_rt/exec.h>
#include <libminix_rt/syscalls.h>
#include <libminix_rt/ipc.h>

int execve(struct minix_rt_binprm *bprm)
{
	if (!bprm)
		return -EINVAL;



	return 0;
}
