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
#ifndef __ASM_MISC_BITOPS_H_
#define __ASM_MISC_BITOPS_H_

#include <misc/compiler.h>

#ifndef __MISC_BITOPS_H_
#error only <misc/bitops.h> can be included directly
#endif

#include <asm-generic/misc/bitops/builtin-__ffs.h>
#include <asm-generic/misc/bitops/builtin-ffs.h>
#include <asm-generic/misc/bitops/builtin-__fls.h>
#include <asm-generic/misc/bitops/builtin-fls.h>

#include <asm-generic/misc/bitops/ffz.h>
#include <asm-generic/misc/bitops/fls64.h>
#include <asm-generic/misc/bitops/find.h>

#include <asm-generic/misc/bitops/hweight.h>

#include <asm-generic/misc/bitops/atomic.h>
#include <asm-generic/misc/bitops/lock.h>
#include <asm-generic/misc/bitops/non-atomic.h>
#include <asm-generic/misc/bitops/le.h>

#endif /* !__ASM_MISC_BITOPS_H_ */
