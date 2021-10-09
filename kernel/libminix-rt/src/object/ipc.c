#include <libminix_rt/object/ipc.h>

static unsigned long minix_rt_ipc_buffer;

void ipc_set_user_space_ptr(unsigned long ipcptr)
{
	minix_rt_ipc_buffer = ipcptr;
}

unsigned long ipc_get_user_space_ptr(void)
{
	return minix_rt_ipc_buffer;
}
