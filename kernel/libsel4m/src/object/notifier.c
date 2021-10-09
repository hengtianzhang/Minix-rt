#include <libminix_rt/object/notifier.h>
#include <libminix_rt/syscalls.h>

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
					SIGCHLD, 0, 0, flags, 0);

	return ret;
}

int notifier_send_notifier(int notifier, pid_t receiver, void *private)
{
	int ret;

	ret = __syscall(__NR_notifier, notifier_send_signal,
					notifier, 0, receiver, (long) private, 0);

	return ret;
}
