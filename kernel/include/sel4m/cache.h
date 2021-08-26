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

#endif /* !__SEL4M_CACHE_H_ */
