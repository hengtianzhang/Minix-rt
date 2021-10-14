#ifndef __LIBMINIX_RT_STDLIB_H_
#define __LIBMINIX_RT_STDLIB_H_

#include <base/types.h>

unsigned long get_free_page(void);

void *malloc(size_t size);
void free(void *addr);

#endif /* !__LIBMINIX_RT_STDLIB_H_ */
