#include <stdio.h>
#include <libsel4m/object/untype.h>
#include <libsel4m/object/tcb.h>
#include <libsel4m/object/notifier.h>

static int test_thread(void *arg)
{
	printf("Hello, This is a test thread pid is %d!\n", tcb_get_pid_info());

	return 0;
}

int main(void)
{
	pid_t pid;
	int ret, i;

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

	for (i = 0; i < 1; i++) {
		pid = tcb_create_thread(test_thread, NULL);
		if (pid)
			printf("create thred sucess pid is %d\n", pid);
	}

	while (1);
	return 0;
}
