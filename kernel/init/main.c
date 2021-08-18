
#include <generated/asm-offsets.h>

#include <muslc/muslc.h>

extern void test1(int d);

void start_kernel(void)
{
	test1(2);
	muslc();
}
