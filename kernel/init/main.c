
#include <generated/asm-offsets.h>

extern void test1(int d);

void start_kernel(void)
{
	test1(2);

}
