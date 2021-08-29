/*
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
#ifndef __ASM_PGTABLE_H_
#define __ASM_PGTABLE_H_

#include <asm/memory.h>
#include <asm/kernel-pgtable.h>

#ifndef __ASSEMBLY__

extern pgd_t init_pg_dir[PTRS_PER_PGD];
extern pgd_t init_pg_end[];
extern pgd_t swapper_pg_dir[PTRS_PER_PGD];
extern pgd_t idmap_pg_dir[PTRS_PER_PGD];

extern pgd_t *kernel_pgd;

#define __pte_to_phys(pte)	(pte_val(pte) & PTE_ADDR_MASK)

static inline pte_t pgd_pte(pgd_t pgd)
{
	return __pte(pgd_val(pgd));
}

#define __pgd_to_phys(pgd)	__pte_to_phys(pgd_pte(pgd))

#if CONFIG_PGTABLE_LEVELS > 3

#define pgd_none(pgd)		(!pgd_val(pgd))
#define pgd_bad(pgd)		(!(pgd_val(pgd) & 2))
#define pgd_present(pgd)	(pgd_val(pgd))

static inline phys_addr_t pgd_page_paddr(pgd_t pgd)
{
	return __pgd_to_phys(pgd);
}

/* Find an entry in the frst-level page table. */
#define pud_index(addr)		(((addr) >> PUD_SHIFT) & (PTRS_PER_PUD - 1))

#define pud_offset_phys(dir, addr)	(pgd_page_paddr(READ_ONCE(*(dir))) + pud_index(addr) * sizeof(pud_t))

/* use ONLY for statically allocated translation tables */
#define pud_offset_kimg(dir,addr)	((pud_t *)__phys_to_kimg(pud_offset_phys((dir), (addr))))

#else

#define pgd_page_paddr(pgd)	({ BUILD_BUG(); 0;})

/*
 * The "pgd_xxx()" functions here are trivial for a folded two-level
 * setup: the pud is never bad, and a pud always exists (as it's folded
 * into the pgd entry)
 */
static inline int pgd_none(pgd_t pgd)		{ return 0; }
static inline int pgd_bad(pgd_t pgd)		{ return 0; }
static inline int pgd_present(pgd_t pgd)	{ return 1; }

#define pud_offset_kimg(dir,addr)	((pud_t *)dir)

#endif  /* CONFIG_PGTABLE_LEVELS > 3 */

/* to find an entry in a page-table-directory */
#define pgd_index(addr)		(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

#define pgd_offset_raw(pgd, addr)	((pgd) + pgd_index(addr))

#define pgd_offset(pgd, addr)	(pgd_offset_raw(pgd, (addr)))

/* to find an entry in a kernel page-table-directory */
#define pgd_offset_k(addr)	pgd_offset(kernel_pgd, addr)

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_PGTABLE_H_ */
