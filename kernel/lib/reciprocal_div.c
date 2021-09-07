#include <sel4m/reciprocal_div.h>

#include <asm/base/div64.h>

u32 reciprocal_value(u32 k)
{
	u64 val = (1LL << 32) + (k - 1);
	do_div(val, k);
	return (u32)val;
}
