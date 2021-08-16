/*
 * Based on arch/arm/kernel/asm-offsets.c
 *
 * Copyright (C) 1995-2003 Russell King
 *               2001-2002 Keith Owens
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
typedef unsigned long long size_t;
#include <linux/kbuild.h>

struct aa {
	int bb;
	long long cc;
};

struct bb {
	unsigned long scs;
	struct aa vv;
	int a;
};

int main(void)
{
	DEFINE(CC,	offsetof(struct aa, cc));
	DEFINE(SDASD,	offsetof(struct bb, a));
	return 0;
}
