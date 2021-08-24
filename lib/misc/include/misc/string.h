/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __MISC_STRING_H_
#define __MISC_STRING_H_

#include <stdarg.h>

#include <misc/compiler.h>	/* for inline */
#include <misc/types.h>	/* for size_t */
#include <misc/stddef.h>	/* for NULL */

/*
 * Include machine specific inline routines
 */
#include <asm/misc/string.h>

#ifndef __HAVE_ARCH_STRCPY
extern char * strcpy(char *,const char *);
#endif
#ifndef __HAVE_ARCH_STRNCPY
extern char * strncpy(char *,const char *, size_t);
#endif
#ifndef __HAVE_ARCH_STRLCPY
size_t strlcpy(char *, const char *, size_t);
#endif
#ifndef __HAVE_ARCH_STRSCPY
ssize_t strscpy(char *, const char *, size_t);
#endif
#ifndef __HAVE_ARCH_STRCAT
extern char * strcat(char *, const char *);
#endif
#ifndef __HAVE_ARCH_STRNCAT
extern char * strncat(char *, const char *, size_t);
#endif
#ifndef __HAVE_ARCH_STRLCAT
extern size_t strlcat(char *, const char *, size_t);
#endif
#ifndef __HAVE_ARCH_STRCMP
extern int strcmp(const char *,const char *);
#endif
#ifndef __HAVE_ARCH_STRNCMP
extern int strncmp(const char *,const char *,size_t);
#endif
#ifndef __HAVE_ARCH_STRCASECMP
extern int strcasecmp(const char *s1, const char *s2);
#endif
#ifndef __HAVE_ARCH_STRNCASECMP
extern int strncasecmp(const char *s1, const char *s2, size_t n);
#endif
#ifndef __HAVE_ARCH_STRCHR
extern char * strchr(const char *,int);
#endif
#ifndef __HAVE_ARCH_STRCHRNUL
extern char * strchrnul(const char *,int);
#endif
#ifndef __HAVE_ARCH_STRNCHR
extern char * strnchr(const char *, size_t, int);
#endif
#ifndef __HAVE_ARCH_STRRCHR
extern char * strrchr(const char *,int);
#endif
extern char * skip_spaces(const char *);

extern char *strim(char *);

static inline char *strstrip(char *str)
{
	return strim(str);
}

#ifndef __HAVE_ARCH_STRSTR
extern char * strstr(const char *, const char *);
#endif
#ifndef __HAVE_ARCH_STRNSTR
extern char * strnstr(const char *, const char *, size_t);
#endif
#ifndef __HAVE_ARCH_STRLEN
extern size_t strlen(const char *);
#endif
#ifndef __HAVE_ARCH_STRNLEN
extern size_t strnlen(const char *,size_t);
#endif
#ifndef __HAVE_ARCH_STRPBRK
extern char * strpbrk(const char *,const char *);
#endif
#ifndef __HAVE_ARCH_STRSEP
extern char * strsep(char **,const char *);
#endif
#ifndef __HAVE_ARCH_STRSPN
extern size_t strspn(const char *,const char *);
#endif
#ifndef __HAVE_ARCH_STRCSPN
extern size_t strcspn(const char *,const char *);
#endif

#ifndef __HAVE_ARCH_MEMSET
extern void * memset(void *,int,size_t);
#endif

#ifndef __HAVE_ARCH_MEMSET16
extern void *memset16(uint16_t *, uint16_t, size_t);
#endif

#ifndef __HAVE_ARCH_MEMSET32
extern void *memset32(uint32_t *, uint32_t, size_t);
#endif

#ifndef __HAVE_ARCH_MEMSET64
extern void *memset64(uint64_t *, uint64_t, size_t);
#endif

static inline void *memset_l(unsigned long *p, unsigned long v,
		size_t n)
{
	if (BITS_PER_LONG == 32)
		return memset32((uint32_t *)p, v, n);
	else
		return memset64((uint64_t *)p, v, n);
}

static inline void *memset_p(void **p, void *v, size_t n)
{
	if (BITS_PER_LONG == 32)
		return memset32((uint32_t *)p, (uintptr_t)v, n);
	else
		return memset64((uint64_t *)p, (uintptr_t)v, n);
}

#ifndef __HAVE_ARCH_MEMCPY
extern void * memcpy(void *,const void *,size_t);
#endif
#ifndef __HAVE_ARCH_MEMMOVE
extern void * memmove(void *,const void *,size_t);
#endif
#ifndef __HAVE_ARCH_MEMSCAN
extern void * memscan(void *,int,size_t);
#endif
#ifndef __HAVE_ARCH_MEMCMP
extern int memcmp(const void *,const void *,size_t);
#endif
#ifndef __HAVE_ARCH_MEMCHR
extern void * memchr(const void *,int,size_t);
#endif
#ifndef __HAVE_ARCH_MEMCPY_MCSAFE
static inline unsigned long memcpy_mcsafe(void *dst,
		const void *src, size_t cnt)
{
	memcpy(dst, src, cnt);
	return 0;
}
#endif
#ifndef __HAVE_ARCH_MEMCPY_FLUSHCACHE
static inline void memcpy_flushcache(void *dst, const void *src, size_t cnt)
{
	memcpy(dst, src, cnt);
}
#endif

void *memchr_inv(const void *s, int c, size_t n);
char *strreplace(char *s, char old, char new);

extern int kstrtobool(const char *s, bool *res);
static inline int strtobool(const char *s, bool *res)
{
	return kstrtobool(s, res);
}

int match_string(const char * const *array, size_t n, const char *string);

/**
 * strstarts - does @str start with @prefix?
 * @str: string to examine
 * @prefix: prefix to look for.
 */
static inline bool strstarts(const char *str, const char *prefix)
{
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

size_t memweight(const void *ptr, size_t bytes);
void memzero_explicit(void *s, size_t count);

/**
 * kbasename - return the last part of a pathname.
 *
 * @path: path to extract the filename from.
 */
static inline const char *kbasename(const char *path)
{
	const char *tail = strrchr(path, '/');
	return tail ? tail + 1 : path;
}

/**
 * str_has_prefix - Test if a string has a given prefix
 * @str: The string to test
 * @prefix: The string to see if @str starts with
 *
 * A common way to test a prefix of a string is to do:
 *  strncmp(str, prefix, sizeof(prefix) - 1)
 *
 * But this can lead to bugs due to typos, or if prefix is a pointer
 * and not a constant. Instead use str_has_prefix().
 *
 * Returns: 0 if @str does not start with @prefix
         strlen(@prefix) if @str does start with @prefix
 */
static __always_inline size_t str_has_prefix(const char *str, const char *prefix)
{
	size_t len = strlen(prefix);
	return strncmp(str, prefix, len) == 0 ? len : 0;
}

#endif /* !__MISC_STRING_H_ */
