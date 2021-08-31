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
#ifndef __ASM_BASE_BITOPS_H_
#define __ASM_BASE_BITOPS_H_

#include <base/compiler.h>

#ifndef __BASE_BITOPS_H_
#error only <base/bitops.h> can be included directly
#endif

#include <asm-generic/base/bitops/builtin-__ffs.h>
#include <asm-generic/base/bitops/builtin-ffs.h>
#include <asm-generic/base/bitops/builtin-__fls.h>
#include <asm-generic/base/bitops/builtin-fls.h>

#include <asm-generic/base/bitops/ffz.h>
#include <asm-generic/base/bitops/fls64.h>
#include <asm-generic/base/bitops/find.h>

#include <asm-generic/base/bitops/sched.h>
#include <asm-generic/base/bitops/hweight.h>

#include <asm-generic/base/bitops/atomic.h>
#include <asm-generic/base/bitops/lock.h>
#include <asm-generic/base/bitops/non-atomic.h>
#include <asm-generic/base/bitops/le.h>

#endif /* !__ASM_BASE_BITOPS_H_ */
