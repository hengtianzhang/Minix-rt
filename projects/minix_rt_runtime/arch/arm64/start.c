#include <base/bug.h>

#include <bootinfo.h>
#include <string.h>

#include <libminix_rt/notifier.h>

void __minix_rt_start_c(void)
{
	int size = __bss_end__ - __bss_start__;

	if (size)
		memset(__bss_start__, 0, size + 8);
}

void __minix_rt_exit_c(unsigned long code)
{
	notifier_send_child_exit(0);
	for (;;);
}
