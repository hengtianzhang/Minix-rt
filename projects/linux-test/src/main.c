#include <stdio.h>
#include <libsel4m/object/untype.h>
#include <libsel4m/object/tcb.h>
#include <libsel4m/object/notifier.h>

unsigned long test_data;
int aaaa = 0;

static void test_notifier(int notifier)
{
	aaaa = 3;
	printf("This is notifier %d\n", notifier);
}

static int test_thread(void *arg)
{
	printf("Hello, This is a test thread!\n");
	printf("arg is 0x%lx\n", *(unsigned long *)arg);

	printf("send notifier sigchld!\n");
	notifier_send_child_exit(0);
	while (1);

	return 0;
}

int main(void)
{
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

	ret = notifier_register_handler(SIGCHLD, test_notifier);

	if (ret)
		printf("notifier register failed!\n");

	test_data = 0xaa55;

	ret = tcb_create_thread(2, test_thread, &test_data);
	printf("ok is %d\n", ret);

	while (1) {
		if (READ_ONCE(aaaa) == 3)
			break;
		cpu_relax();
	}
	printf("OK handler signal!\n");

	return 0;
}
