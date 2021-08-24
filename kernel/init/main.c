
#include <generated/asm-offsets.h>
#include <misc/compiler.h>
#include <misc/types.h>
#include <misc/common.h>
#include <misc/atomic.h>


#include <asm/barrier.h>
#include <linux/bitops.h>

phys_addr_t bbb;
void start_kernel(void)
{
	int aaa=22,ccc=3;
	do_div(aaa, ccc);
	bbb = PHYS_ADDR_MAX;
}
