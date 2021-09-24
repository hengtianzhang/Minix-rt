#include <libsel4m/object/notifier.h>
#include <libsel4m/syscalls.h>

int notifier_register_handler(unsigned int notifier, __sighandler_t fn)
{
	int ret;

	ret = __syscall(__NR_notifier, notifier_regiser_fn,
					notifier, (unsigned long)fn, 0, 0, __kernel_rt_sigreturn);

	return ret;
}

int notifier_send_child_exit(int flags)
{
	int ret;

	ret = __syscall(__NR_notifier, notifier_send_signal,
					SIGCHLD, 0, 0, flags);

	return ret;
}
