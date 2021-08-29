#ifndef __SEL4M_CACHE_H_
#define __SEL4M_CACHE_H_

#include <asm/cache.h>

#ifndef SMP_CACHE_BYTES
#define SMP_CACHE_BYTES L1_CACHE_BYTES
#endif

#ifndef __cacheline_aligned
#define __cacheline_aligned					\
  __attribute__((__aligned__(SMP_CACHE_BYTES),			\
		 __section__(".data..cacheline_aligned")))
#endif /* !__cacheline_aligned */

/*
 * __ro_after_init is used to mark things that are read-only after init (i.e.
 * after mark_rodata_ro() has been called). These are effectively read-only,
 * but may get written to during init, so can't live in .rodata (via "const").
 */
#ifndef __ro_after_init
#define __ro_after_init __attribute__((__section__(".data..ro_after_init")))
#endif

#endif /* !__SEL4M_CACHE_H_ */
