/*
 *  linux/kernel/reboot.c
 *
 *  Copyright (C) 2013  Linus Torvalds
 */

#include <sel4m/reboot.h>

#define DEFAULT_REBOOT_MODE
enum reboot_mode reboot_mode DEFAULT_REBOOT_MODE;
