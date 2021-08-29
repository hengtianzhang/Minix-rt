#ifndef __SEL4M_STRING_H_
#define __SEL4M_STRING_H_

#include <asm/string.h>

#ifndef __HAVE_ARCH_STRNLEN
extern size_t strnlen(const char *,size_t);
#endif

#endif /* !__SEL4M_STRING_H_ */
