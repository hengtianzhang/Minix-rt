/*
 *  linux/init/main.c
 *
 *  Copyright (C) 1991, 1992  Linus Torvalds
 *
 *  GK 2/5/95  -  Changed to support mounting root fs via NFS
 *  Added initrd & change_root: Werner Almesberger & Hans Lermen, Feb '96
 *  Moan early if gcc is old, avoiding bogus kernels - Paul Gortmaker, May '96
 *  Simplified starting of init:  Michael A. Griffith <grif@acm.org>
 */
#include <sel4m/init.h>
#include <sel4m/compiler.h>
#include <sel4m/linkage.h>
#include <sel4m/kernel.h>

#include <memalloc/mmzone.h>
#include <memalloc/memblock.h>
#include <base/common.h>
#include <base/list.h>
#include <memalloc/mmzone.h>

#include <asm/memory.h>

void __weak __init early_arch_platform_init(void) {}

struct memblock memblock;

asmlinkage __visible void __init start_kernel(void)
{
    int i;
    phys_addr_t start_pfn, end_pfn;

    early_arch_platform_init();
    printf("sssss fdd 0x%llx\n", FIXADDR_TOP);

    memblock_init(&memblock);
    memblock_add(&memblock, 0x40000000, 0x10000000);
    memblock_add(&memblock, 0x40000000, 0x100000);
    memblock_add(&memblock, 0x80000000, 0x1060000);
    memblock_dump_all(&memblock);
    printf("ssssssssssss 0x%lx\n", BIT(VA_BITS - 1));
    for_each_mem_pfn_range(&memblock, i, &start_pfn, &end_pfn)
        printf("i = %d, start 0x%llx end 0x%llx\n",i, start_pfn, end_pfn);
    
    hang("ssdasdas\n");
    while (1);
}
