#include <libminix_rt/object/ipc.h>

#include <base/bug.h>

#include <bootinfo.h>
#include <string.h>

void __minix_rt_start_c(unsigned long ipcptr)
{
	int size = __bss_end__ - __bss_start__;

	if (!size)
		memset(__bss_start__, 0, size);

	ipc_set_user_space_ptr(ipcptr);
}
