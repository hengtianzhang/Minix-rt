#include <base/init.h>
#include <base/common.h>

extern char __start_archive[];
extern char __end_archive[];

void __init service_core_init(void)
{
	printf("ssssss %p\n", __start_archive);
	printf("ssssss %p\n", __end_archive);
}
