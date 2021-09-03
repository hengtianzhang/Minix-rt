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
    early_arch_platform_init();
    printf("sssss fdd 0x%llx\n", FIXADDR_TOP);

    while (1);
}
