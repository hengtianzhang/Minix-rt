/*
 * Generic helpers for smp ipi calls
 *
 * (C) Jens Axboe <jens.axboe@oracle.com> 2008
 */

#include <sel4m/linkage.h>
#include <sel4m/smp.h>

/*
 * as from 2.5, kernels no longer have an init_tasks structure
 * so we need some other way of telling a new secondary core
 * where to place its SVC stack
 */
struct secondary_data secondary_data;

/*
 * This is the secondary CPU boot entry.  We're using this CPUs
 * idle thread stack, but a set of temporary page tables.
 */
asmlinkage void secondary_start_kernel(void)
{
}
