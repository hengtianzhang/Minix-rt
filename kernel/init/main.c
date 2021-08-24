
#include <generated/asm-offsets.h>
#include <misc/compiler.h>
#include <misc/types.h>


int bbb;
void start_kernel(void)
{
	int a[20];
	__must_be_array(a);
	bbb = CONFIG_NR_CPUS;
}
