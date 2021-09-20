#include <libsel4m/object/ipc.h>

#include <bootinfo.h>
#include <string.h>

void __sel4m_start_c(unsigned long ipcptr)
{
	int size = __bss_end__ - __bss_start__;

	if (!size)
		memset(__bss_start__, 0, size);

	sel4m_ipc_buffer = ipcptr;
}
