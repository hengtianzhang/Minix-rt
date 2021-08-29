/*
 * Based on arch/arm/mm/mmu.c
 *
 * Copyright (C) 1995-2005 Russell King
 * Copyright (C) 2012 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <sel4m/types.h>
#include <sel4m/init.h>
#include <sel4m/page.h>

#include <asm/memory.h>
#include <asm/fixmap.h>
#include <asm/pgtable.h>

u64 idmap_t0sz = TCR_T0SZ(VA_BITS);
u64 idmap_ptrs_per_pgd = PTRS_PER_PGD;

u64 kimage_voffset;

pgd_t *kernel_pgd = init_pg_dir;

static pte_t bm_pte[PTRS_PER_PTE] __page_aligned_bss;
static pmd_t bm_pmd[PTRS_PER_PMD] __page_aligned_bss __maybe_unused;
static pud_t bm_pud[PTRS_PER_PUD] __page_aligned_bss __maybe_unused;

void __init early_fixmap_init(void)
{
    pgd_t *pgdp, pgd;
	pud_t *pudp;
	pmd_t *pmdp;
    u64 addr = FIXADDR_START;

    pgdp = pgd_offset_k(addr);
	pgd = READ_ONCE(*pgdp);
	if (CONFIG_PGTABLE_LEVELS > 3 &&
	    !(pgd_none(pgd) || pgd_page_paddr(pgd) == __pa_symbol(bm_pud))) {
		/*
		 * We only end up here if the kernel mapping and the fixmap
		 * share the top level pgd entry, which should only happen on
		 * 16k/4 levels configurations.
		 */
		BUG_ON(!IS_ENABLED(CONFIG_ARM64_16K_PAGES));
		pudp = pud_offset_kimg(pgdp, addr);
	}
}
