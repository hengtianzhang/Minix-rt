#ifndef __LIBMINIX_RT_STRING_H_
#define __LIBMINIX_RT_STRING_H_

#include <base/string.h>

extern int message_memcpy(void *dst, const void *src, int size, pid_t src_pid);

extern int message_strlen(const void *s, pid_t src_pid);

#endif /* !__LIBMINIX_RT_STRING_H_ */
