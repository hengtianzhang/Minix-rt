#include <libsel4m/object/tcb.h>
#include <libsel4m/syscalls.h>

pid_t tcb_create_thread(pid_t pid, tcb_thread_fn_t fn, void *arg)
{
	pid_t ret;

	ret = __syscall(__NR_tcb_thread, tcb_create_thread_fn,
				pid, (unsigned long)fn, (unsigned long)arg);

	return ret;
}
