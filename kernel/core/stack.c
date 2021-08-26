
#include <sel4m/smp.h>

__visible __attribute__((aligned(THREAD_STACK_ALIGN)))
u8 kernel_stack_alloc[CONFIG_NR_CPUS][THREAD_SIZE] __section(.data..kernel_stack);
