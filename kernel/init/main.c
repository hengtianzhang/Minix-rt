
#include <generated/asm-offsets.h>

#include <muslc/muslc.h>

extern void test1(int d);

int bbb;
void start_kernel(void)
{
	int a;

	test1(2);
	muslc();

	bbb = CONFIG_NR_CPUS;
}
