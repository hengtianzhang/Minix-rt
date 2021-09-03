// SPDX-License-Identifier: GPL-2.0
#include <sel4m/mm_types.h>

#include <asm/pgtable.h>
#include <asm/mmu.h>

#ifndef INIT_MM_CONTEXT
#define INIT_MM_CONTEXT(name)
#endif

struct mm_struct init_mm = {
    .pgd		= swapper_pg_dir,
    INIT_MM_CONTEXT(init_mm)
};
