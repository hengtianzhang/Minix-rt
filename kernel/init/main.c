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

#include <memblock/memblock.h>
#include <base/common.h>

#include <asm/memory.h>

void __weak __init early_arch_platform_init(void) {}

struct memblock memblock;

asmlinkage __visible void __init start_kernel(void)
{
    early_arch_platform_init();
    printf("sssss fdd 0x%llx\n", FIXADDR_TOP);

    memblock_debug_enable();
    memblock_init(&memblock);
    memblock_add(&memblock, 0x300, 0x100);
    memblock_dump_all(&memblock);
    while (1);
}
