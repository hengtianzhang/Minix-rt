#include <minix_rt/syscalls.h>

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
