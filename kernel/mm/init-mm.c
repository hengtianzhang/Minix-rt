// SPDX-License-Identifier: GPL-2.0
#include <sel4m/mm_types.h>

#include <asm/pgtable.h>
#include <asm/mmu.h>

#ifndef INIT_MM_CONTEXT
#define INIT_MM_CONTEXT(name)
#endif

struct mm_struct init_mm = {
    .vma_rb_root = RB_ROOT,
    .vma_lock   = __SPIN_LOCK_UNLOCKED(init_mm.vma_lock),
    .pgd		= swapper_pg_dir,
    .pgtables_bytes = ATOMIC_LONG_INIT(0),
    .page_table_lock = __SPIN_LOCK_UNLOCKED(init_mm.page_table_lock),
    INIT_MM_CONTEXT(init_mm)
};
