#include <stdio.h>
#include <libsel4m/object/untype.h>

int main(void)
{
	int ret;

	printf("This rootServices!\n");

	ret = untype_alloc_area(0x1000, 0x10000, VM_WRITE | VM_READ);
	if (ret)
		printf("alloc untype failed!\n");

	ret = untype_vmap_area(0x1000);
	*(u64 *)0x1000 = 0x3;
	untype_vunmap_area(0x1000);
	untype_free_area(0x1000);
	printf("SSSSSSSSSSSSS %d\n", ret);

	return 0;
}
