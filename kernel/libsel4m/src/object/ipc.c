#include <libsel4m/object/ipc.h>

static unsigned long sel4m_ipc_buffer;

void ipc_set_user_space_ptr(unsigned long ipcptr)
{
	sel4m_ipc_buffer = ipcptr;
}
