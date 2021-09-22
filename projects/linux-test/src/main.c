#include <stdio.h>
#include <libsel4m/object/untype.h>
#include <libsel4m/object/tcb.h>

unsigned long test_data;

static int test_thread(void *arg)
{
	printf("Hello, This is a test thread!\n");
	printf("arg is 0x%lx\n", *(unsigned long *)arg);

	while (1);

	return 0;
}

int main(void)
{
	pid_t pid;
	int ret;

	printf("This rootServices!\n");

	ret = untype_alloc_area(0x1000, 0x10000, VM_WRITE | VM_READ);
	if (ret)
		printf("alloc untype failed!\n");

	ret = untype_vmap_area(0x1000);
	if (ret) {
		printf("vmap untype failed\n");
		untype_free_area(0x1000);

		return -1;
	}

	*(u64 *)0x1000 = 0x3;
	untype_vunmap_area(0x1000);
	untype_free_area(0x1000);
	printf("Finish vmap test!\n");

	test_data = 0xaa55;

	pid = tcb_create_thread(2, test_thread, &test_data);
	printf("pid is %d\n", pid);

	return 0;
}
