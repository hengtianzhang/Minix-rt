#include <sel4m/object/notifier.h>
#include <sel4m/syscalls.h>
#include <sel4m/sched.h>
#include <sel4m/signal.h>

#include <uapi/sel4m/object/notifier.h>
#include <uapi/sel4m/object/cap_types.h>

SYSCALL_DEFINE6(notifier, enum notifier_type, table,
				unsigned int, notifier, unsigned long, fn,
				pid_t, pid, int, flags, __sigrestore_t, return_fn)
{
	if (notifier == NOTIFIER_MESSAGE) {
		if (!cap_table_test_cap(cap_notification_cap, &current->cap_table))
			return -ENONOTIFIER;
	}

	if (notifier >= NOTIFIER_NR_MAX)
		return -EINVAL;

	switch (table) {
		case notifier_regiser_fn:
			if (BAD_ADDR(fn))
				return -EFAULT;

			current->notifier.action[notifier].sa.sa_handler = (__sighandler_t)fn;
			if (current->notifier.return_fn)
				BUG_ON(current->notifier.return_fn != return_fn);
			current->notifier.return_fn = return_fn;
			return 0;
		case notifier_remove_fn:
			printf("remove notifier!\n");
			return 0;
		case notifier_send_signal:
			if (notifier < SIGRTMAX)
				return do_send_signal(notifier, pid, flags);
			else
				printf("send notifier %d nothing TODO\n", notifier);
			return 0;
	}

	return -EINVAL;
}
