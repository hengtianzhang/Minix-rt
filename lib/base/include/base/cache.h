#ifndef __BASE_CACHE_H_
#define __BASE_CACHE_H_

#include <base/common.h>

#include <asm/base/cache.h>

#ifndef L1_CACHE_ALIGN
#define L1_CACHE_ALIGN(x) __ALIGN_COMMON(x, L1_CACHE_BYTES)
#endif

#ifndef SMP_CACHE_BYTES
#define SMP_CACHE_BYTES L1_CACHE_BYTES
#endif

/*
 * __read_mostly is used to keep rarely changing variables out of frequently
 * updated cachelines. If an architecture doesn't support it, ignore the
 * hint.
 */
#ifndef __read_mostly
#define __read_mostly
#endif

/*
 * The maximum alignment needed for some critical structures
 * These could be inter-node cacheline sizes/L3 cacheline
 * size etc.  Define this in asm/cache.h for your arch
 */
#ifndef INTERNODE_CACHE_SHIFT
#define INTERNODE_CACHE_SHIFT L1_CACHE_SHIFT
#endif

#if !defined(____cacheline_internodealigned_in_smp)
#define ____cacheline_internodealigned_in_smp \
	__attribute__((__aligned__(1 << (INTERNODE_CACHE_SHIFT))))
#endif

#endif /* !__BASE_CACHE_H_ */
