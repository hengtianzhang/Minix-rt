#include <libsel4m/object/tcb.h>
#include <libsel4m/object/notifier.h>
#include <libsel4m/syscalls.h>

void __sel4m_exit_c(void)
{
	notifier_send_child_exit(0);
	BUG();
}

pid_t tcb_create_thread(tcb_thread_fn_t fn, void *arg)
{
	int ret;

	ret = __syscall(__NR_tcb_thread, tcb_create_thread_fn,
				(unsigned long)fn, (unsigned long)arg,
				(unsigned long) __sel4m_exit_c);

	return ret;
}

pid_t tcb_get_pid_info(void)
{
	pid_t pid;

	pid = __syscall(__NR_tcb_thread, tcb_get_pid, 0, 0, 0);

	return pid;
}
