#include <bootinfo.h>
#include <string.h>

struct bootinfo bootinfo;

void __sel4m_start_c(struct bootinfo *binfo)
{
	int size = __bss_end__ - __bss_start__;

	if (!size)
		memset(__bss_start__, 0, size);
}
