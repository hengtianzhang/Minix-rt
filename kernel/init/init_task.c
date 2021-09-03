#include <sel4m/sched.h>
#include <sel4m/mm_types.h>

struct task_struct init_task = {
	.state		= 0,
	.mm			= &init_mm,
};
