#ifndef __ASM_BASE_AUXVEC_H_
#define __ASM_BASE_AUXVEC_H_

/* vDSO location */
#define AT_SYSINFO_EHDR	33
#define AT_MINSIGSTKSZ	51	/* stack needed for signal delivery */

#define AT_VECTOR_SIZE_ARCH 2 /* entries in ARCH_DLINFO */

#endif /* !__ASM_BASE_AUXVEC_H_ */
