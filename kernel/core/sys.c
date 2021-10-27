#include <minix_rt/syscalls.h>
#include <minix_rt/uts.h>
#include <minix_rt/sched.h>

SYSCALL_DEFINE0(getpid)
{
	return current->pid.pid;
}

SYSCALL_DEFINE0(getuid)
{
	/* Only we change this so SMP safe */
	return 0;
}

SYSCALL_DEFINE0(geteuid)
{
	/* Only we change this so SMP safe */
	return 0;
}

SYSCALL_DEFINE0(getgid)
{
	/* Only we change this so SMP safe */
	return 0;
}

SYSCALL_DEFINE0(getegid)
{
	/* Only we change this so SMP safe */
	return 0;
}

SYSCALL_DEFINE1(newuname, struct new_utsname __user *, name)
{
	struct new_utsname tmp;

	memcpy(&tmp, &utsname, sizeof(tmp));
	if (copy_to_user(name, &tmp, sizeof(tmp)))
		return -EFAULT;

	return 0;
}
