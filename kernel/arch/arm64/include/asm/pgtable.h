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

/* Find an entry in the third-level page table. */
#define pte_index(addr)		(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))

#define pte_valid(pte)		(!!(pte_val(pte) & PTE_VALID))

#define __pte_to_phys(pte)	(pte_val(pte) & PTE_ADDR_MASK)

#define pgd_index(addr)	(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

#define pgd_offset(pgd, addr) ((pgd) + pgd_index(addr))

#define pgd_offset_k(addr)	pgd_offset(kernel_pgd, addr)

#define pgd_none(pgd)		(!pgd_val(pgd))
#define pgd_bad(pgd)		(!(pgd_val(pgd) & 2))
#define pgd_present(pgd)	(pgd_val(pgd))

static inline pte_t pgd_pte(pgd_t pgd)
{
	return __pte(pgd_val(pgd));
}

static inline pte_t pud_pte(pud_t pud)
{
	return __pte(pud_val(pud));
}

#define __pgd_to_phys(pgd)	__pte_to_phys(pgd_pte(pgd))

static inline void __pgd_populate(pgd_t *pgdp, phys_addr_t pudp, pgdval_t prot)
{
	WRITE_ONCE(*pgdp, __pgd(pudp | prot));
	dsb(ishst);
}

static inline phys_addr_t pgd_page_paddr(pgd_t pgd)
{
	return __pgd_to_phys(pgd);
}

/* Find an entry in the frst-level page table. */
#define pud_index(addr)		(((addr) >> PUD_SHIFT) & (PTRS_PER_PUD - 1))

#define pud_offset_phys(dir, addr)	(pgd_page_paddr(READ_ONCE(*(dir))) + pud_index(addr) * sizeof(pud_t))

/* use ONLY for statically allocated translation tables */
#define pud_offset_kimg(dir,addr)	((pud_t *)__phys_to_kimg(pud_offset_phys((dir), (addr))))

#define pud_none(pud)		(!pud_val(pud))
#define pud_bad(pud)		(!(pud_val(pud) & PUD_TABLE_BIT))
#define pud_valid(pud)		pte_valid(pud_pte(pud))

static inline void __pud_populate(pud_t *pudp, phys_addr_t pmdp, pudval_t prot)
{
	WRITE_ONCE(*pudp, __pud(pmdp | prot));

	if (pud_valid(__pud(pmdp | prot)))
		dsb(ishst);
}

#define __pud_to_phys(pud)	__pte_to_phys(pud_pte(pud))

static inline phys_addr_t pud_page_paddr(pud_t pud)
{
	return __pud_to_phys(pud);
}

/* Find an entry in the second-level page table. */
#define pmd_index(addr)		(((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))

#define pmd_offset_phys(dir, addr)	(pud_page_paddr(READ_ONCE(*(dir))) + pmd_index(addr) * sizeof(pmd_t))

/* use ONLY for statically allocated translation tables */
#define pmd_offset_kimg(dir,addr)	((pmd_t *)__phys_to_kimg(pmd_offset_phys((dir), (addr))))

static inline void __pmd_populate(pmd_t *pmdp, phys_addr_t ptep,
				  pmdval_t prot)
{
	WRITE_ONCE(*pmdp, __pmd(ptep | prot));
}

#define vmemmap			((struct page *)VMEMMAP_START - (memstart_addr >> PAGE_SHIFT))

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_PGTABLE_H_ */
