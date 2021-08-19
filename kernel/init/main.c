
#include <generated/asm-offsets.h>

#include <muslc/muslc.h>

#include <misc/misc.h>

extern void test1(int d);

int bbb;
void start_kernel(void)
{
	test1(2);
	muslc();

	bbb = CONFIG_NR_CPUS;
}
