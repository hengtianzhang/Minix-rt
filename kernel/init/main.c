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
#include <base/linkage.h>
#include <base/init.h>
#include <base/compiler.h>
#include <base/common.h>
#include <base/list.h>

#include <sel4m/memory.h>

void __weak __init early_arch_platform_init(void) {}

asmlinkage __visible void __init start_kernel(void)
{
    int i;
    phys_addr_t start_pfn, end_pfn;

    early_arch_platform_init();
    printf("sssss fdd 0x%llx\n", FIXADDR_TOP);

    memblock_init(&memblock_kernel);
    memblock_add(&memblock_kernel, 0x40000000, 0x10000000);
    memblock_add(&memblock_kernel, 0x40000000, 0x100000);
    memblock_add(&memblock_kernel, 0x80000000, 0x1060000);
    memblock_dump_all(&memblock_kernel);

    for_each_mem_pfn_range(&memblock_kernel, i, &start_pfn, &end_pfn)
        printf("i = %d, start 0x%llx end 0x%llx\n",i, start_pfn, end_pfn);

    hang("ssdasdas\n");
    while (1);
}
