#ifndef __ASM_BASE_CACHE_H_
#define __ASM_BASE_CACHE_H_

#define L1_CACHE_SHIFT		(6)
#define L1_CACHE_BYTES		(1 << L1_CACHE_SHIFT)

#define __read_mostly __attribute__((__section__(".data..read_mostly")))

#endif /* !__ASM_BASE_CACHE_H_ */
