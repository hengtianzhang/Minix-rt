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
/*
 * ZERO_PAGE is a global shared page that is always zero: used
 * for zero-mapped memory areas etc..
 */
extern unsigned long empty_zero_page[PAGE_SIZE / sizeof(unsigned long)];

/*
 * Page vaild
 */
/**************************************************************************/
#define pte_valid(pte)		(!!(pte_val(pte) & PTE_VALID))
#define pmd_valid(pmd)		pte_valid(__pte(pmd_val(pmd)))
#define pud_valid(pud)		pte_valid(__pte(pud_val(pud)))

#define pte_valid_not_user(pte) \
	((pte_val(pte) & (PTE_VALID | PTE_USER | PTE_UXN)) == (PTE_VALID | PTE_UXN))

/*
 * Page table index
 */
/**************************************************************************/
/* Find an entry in the third-level page table. */
#define pte_index(addr)		(((addr) >> PAGE_SHIFT) & (PTRS_PER_PTE - 1))
/* Find an entry in the second-level page table. */
#define pmd_index(addr)		(((addr) >> PMD_SHIFT) & (PTRS_PER_PMD - 1))
/* Find an entry in the frst-level page table. */
#define pud_index(addr)		(((addr) >> PUD_SHIFT) & (PTRS_PER_PUD - 1))

#define pgd_index(addr)	(((addr) >> PGDIR_SHIFT) & (PTRS_PER_PGD - 1))

/*
 * Set Page talbe
 */
/**************************************************************************/
static inline void set_pte(pte_t *ptep, pte_t pte)
{
	WRITE_ONCE(*ptep, pte);

	/*
	 * Only if the new pte is valid and kernel, otherwise TLB maintenance
	 * or update_mmu_cache() have the necessary barriers.
	 */
	if (pte_valid_not_user(pte))
		dsb(ishst);
}

static inline void set_pmd(pmd_t *pmdp, pmd_t pmd)
{
	WRITE_ONCE(*pmdp, pmd);

	if (pmd_valid(pmd))
		dsb(ishst);
}

static inline void set_pud(pud_t *pudp, pud_t pud)
{
	WRITE_ONCE(*pudp, pud);

	if (pud_valid(pud))
		dsb(ishst);
}

static inline void set_pgd(pgd_t *pgdp, pgd_t pgd)
{
	WRITE_ONCE(*pgdp, pgd);
	dsb(ishst);
}

/*
 * Populate Page table
 */
/**************************************************************************/
static inline void __pmd_populate(pmd_t *pmdp, phys_addr_t ptep,
				  pmdval_t prot)
{
	set_pmd(pmdp, __pmd(ptep | prot));
}

static inline void __pud_populate(pud_t *pudp, phys_addr_t pmdp, pudval_t prot)
{
	set_pud(pudp, __pud(pmdp | prot));
}

static inline void __pgd_populate(pgd_t *pgdp, phys_addr_t pudp, pgdval_t prot)
{
	set_pgd(pgdp, __pgd(pudp | prot));
}

/*
 * Page table convert
 */
/**************************************************************************/
static inline pte_t pgd_pte(pgd_t pgd)
{
	return __pte(pgd_val(pgd));
}

static inline pte_t pud_pte(pud_t pud)
{
	return __pte(pud_val(pud));
}

static inline pud_t pte_pud(pte_t pte)
{
	return __pud(pte_val(pte));
}

static inline pmd_t pud_pmd(pud_t pud)
{
	return __pmd(pud_val(pud));
}

static inline pte_t pmd_pte(pmd_t pmd)
{
	return __pte(pmd_val(pmd));
}

static inline pmd_t pte_pmd(pte_t pte)
{
	return __pmd(pte_val(pte));
}

/*
 * mk sect prot
 */
/**************************************************************************/
static inline pgprot_t mk_sect_prot(pgprot_t prot)
{
	return __pgprot(pgprot_val(prot) & ~PTE_TABLE_BIT);
}

/*
 * Page table offset
 */
/**************************************************************************/
#define pgd_offset(pgd, addr) ((pgd) + pgd_index(addr))
#define pgd_offset_k(addr)	pgd_offset(kernel_pgd, addr)

/*
 *	Page none
 */
/**************************************************************************/
#define pgd_none(pgd)		(!pgd_val(pgd))
#define pud_none(pud)		(!pud_val(pud))
#define pmd_none(pmd)		(!pmd_val(pmd))
#define pte_none(pte)		(!pte_val(pte))

/*
 *	Page bad
 */
/**************************************************************************/
#define pmd_bad(pmd)		(!(pmd_val(pmd) & PMD_TABLE_BIT))
#define pud_bad(pud)		(!(pud_val(pud) & PUD_TABLE_BIT))
#define pgd_bad(pgd)		(!(pgd_val(pgd) & 2))

/*
 *	Sect or table ?
 */
/**************************************************************************/
#define pmd_table(pmd)		((pmd_val(pmd) & PMD_TYPE_MASK) == \
				 PMD_TYPE_TABLE)
#define pmd_sect(pmd)		((pmd_val(pmd) & PMD_TYPE_MASK) == \
				 PMD_TYPE_SECT)

#define pud_sect(pud)		((pud_val(pud) & PUD_TYPE_MASK) == \
				 PUD_TYPE_SECT)
#define pud_table(pud)		((pud_val(pud) & PUD_TYPE_MASK) == \
				 PUD_TYPE_TABLE)

/*
 *	Page to phys
 */
/**************************************************************************/
#define __pte_to_phys(pte)	(pte_val(pte) & PTE_ADDR_MASK)
#define __pmd_to_phys(pmd)	__pte_to_phys(pmd_pte(pmd))
#define __pud_to_phys(pud)	__pte_to_phys(pud_pte(pud))
#define __pgd_to_phys(pgd)	__pte_to_phys(pgd_pte(pgd))

/*
 * pfn to pte ...
 */
/**************************************************************************/
#define pfn_pud(pfn,prot)	\
	__pud((phys_addr_t)(pfn) << PAGE_SHIFT | pgprot_val(prot))
#define pfn_pmd(pfn,prot)	\
	__pmd((phys_addr_t)(pfn) << PAGE_SHIFT | pgprot_val(prot))
#define pfn_pte(pfn,prot)	\
	__pte((phys_addr_t)(pfn) << PAGE_SHIFT | pgprot_val(prot))

/*
 *	Page table clear
 */
/**************************************************************************/
#define pte_clear(pgd,addr,ptep)	set_pte(ptep, __pte(0))
#define pmd_clear(pgd,addr,pmdp)	set_pmd(pmdp, __pmd(0))
#define pud_clear(pgd,addr,pudp)	set_pud(pudp, __pud(0))
#define pgd_clear(pgd,addr,pgdp)	set_pgd(pgdp, __pgd(0))

/*
 *	Page to paddr
 */
/**************************************************************************/
static inline phys_addr_t pmd_page_paddr(pmd_t pmd)
{
	return __pmd_to_phys(pmd);
}

static inline phys_addr_t pud_page_paddr(pud_t pud)
{
	return __pud_to_phys(pud);
}

static inline phys_addr_t pgd_page_paddr(pgd_t pgd)
{
	return __pgd_to_phys(pgd);
}

/*
 *	Pgtalbe to offset
 */
/**************************************************************************/
#define pte_offset_phys(dir,addr)	(pmd_page_paddr(READ_ONCE(*(dir))) + pte_index(addr) * sizeof(pte_t))
#define pte_offset_kernel(dir,addr)	((pte_t *)__va(pte_offset_phys((dir), (addr))))

#define pmd_offset_phys(dir, addr)	(pud_page_paddr(READ_ONCE(*(dir))) + pmd_index(addr) * sizeof(pmd_t))
/* use ONLY for statically allocated translation tables */
#define pmd_offset_kimg(dir,addr)	((pmd_t *)__phys_to_kimg(pmd_offset_phys((dir), (addr))))

#define pud_offset_phys(dir, addr)	(pgd_page_paddr(READ_ONCE(*(dir))) + pud_index(addr) * sizeof(pud_t))
/* use ONLY for statically allocated translation tables */
#define pud_offset_kimg(dir,addr)	((pud_t *)__phys_to_kimg(pud_offset_phys((dir), (addr))))

/*
 * Page table Set fixmap
 */
/**************************************************************************/
#define pte_set_fixmap(addr)		((pte_t *)set_fixmap_offset(FIX_PTE, addr))
#define pte_set_fixmap_offset(pmd, addr)	pte_set_fixmap(pte_offset_phys(pmd, addr))
#define pte_clear_fixmap()		clear_fixmap(FIX_PTE)

#define pmd_set_fixmap(addr)		((pmd_t *)set_fixmap_offset(FIX_PMD, addr))
#define pmd_set_fixmap_offset(pud, addr)	pmd_set_fixmap(pmd_offset_phys(pud, addr))
#define pmd_clear_fixmap()		clear_fixmap(FIX_PMD)

#define pud_set_fixmap(addr)		((pud_t *)set_fixmap_offset(FIX_PUD, addr))
#define pud_set_fixmap_offset(pgd, addr)	pud_set_fixmap(pud_offset_phys(pgd, addr))
#define pud_clear_fixmap()		clear_fixmap(FIX_PUD)

#define pgd_set_fixmap(addr)	((pgd_t *)set_fixmap_offset(FIX_PGD, addr))
#define pgd_clear_fixmap()	clear_fixmap(FIX_PGD)

/*
 * When walking page tables, get the address of the next boundary,
 * or the end address of the range if that comes earlier.  Although no
 * vma end wraps to 0, rounded up __boundary may wrap to 0 throughout.
 */
/**************************************************************************/
#define pmd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PMD_SIZE) & PMD_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

#define pud_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PUD_SIZE) & PUD_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

#define pgd_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + PGDIR_SIZE) & PGDIR_MASK;	\
	(__boundary - 1 < (end) - 1)? __boundary: (end);		\
})

/*
 * cont addr walking
 */
/**************************************************************************/
#define pte_cont_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + CONT_PTE_SIZE) & CONT_PTE_MASK;	\
	(__boundary - 1 < (end) - 1) ? __boundary : (end);			\
})

#define pmd_cont_addr_end(addr, end)						\
({	unsigned long __boundary = ((addr) + CONT_PMD_SIZE) & CONT_PMD_MASK;	\
	(__boundary - 1 < (end) - 1) ? __boundary : (end);			\
})

/*
 *	Define vmemmap 
 */
/**************************************************************************/
#define vmemmap			((struct page *)VMEMMAP_START - (memstart_addr >> PAGE_SHIFT))

int pud_set_huge(pud_t *pud, phys_addr_t addr, pgprot_t prot);
int pmd_set_huge(pmd_t *pmdp, phys_addr_t phys, pgprot_t prot);
int pud_clear_huge(pud_t *pudp);
int pmd_clear_huge(pmd_t *pmdp);

#endif /* !__ASSEMBLY__ */
#endif /* !__ASM_PGTABLE_H_ */
