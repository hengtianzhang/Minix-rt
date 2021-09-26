#include <libsel4m/object/tcb.h>
#include <libsel4m/object/notifier.h>
#include <libsel4m/syscalls.h>

void __sel4m_exit_c(void)
{
	notifier_send_child_exit(0);
	BUG();
}

int tcb_create_thread(pid_t pid, tcb_thread_fn_t fn, void *arg)
{
	int ret;

	ret = __syscall(__NR_tcb_thread, tcb_create_thread_fn,
				pid, (unsigned long)fn, (unsigned long)arg,
				(unsigned long) __sel4m_exit_c);

	return ret;
}
