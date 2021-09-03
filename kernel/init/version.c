/*
 *  linux/init/version.c
 *
 *  Copyright (C) 1992  Theodore Ts'o
 *
 *  May be freely distributed as part of Linux.
 */

#include <generated/compile.h>
#include <generated/version.h>

#include <sel4m/build-salt.h>
#include <sel4m/uts.h>

#define version(a) Version_ ## a
#define version_string(a) version(a)

extern int version_string(SEL4M_VERSION_CODE);
int version_string(SEL4M_VERSION_CODE);

/* FIXED STRINGS! Don't touch! */
const char linux_banner[] =
	"Sel4m version " KERNEL_VERSION_STRING " (" KERNEL_COMPILE_BY "@"
	KERNEL_COMPILE_HOST ") (" KERNEL_COMPILER ") " UTS_VERSION "\n";

BUILD_SALT;
