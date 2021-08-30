/* SPDX-License-Identifier: GPL-2.0 */
#ifndef __BASE_STRING_H_
#define __BASE_STRING_H_

#include <stdarg.h>
#include <base/compiler.h>	/* for inline */
#include <base/types.h>	/* for size_t */
#include <base/stddef.h>	/* for NULL */

/*
 * Include machine specific inline routines
 */
#include <asm/base/string.h>

#ifndef __HAVE_ARCH_STRNCASECMP
extern int strncasecmp(const char *s1, const char *s2, size_t n);
#endif
#ifndef __HAVE_ARCH_STRCASECMP
extern int strcasecmp(const char *s1, const char *s2);
#endif
#ifndef __HAVE_ARCH_STRCPY
extern char * strcpy(char *,const char *);
#endif
#ifndef __HAVE_ARCH_STRNCPY
extern char * strncpy(char *,const char *, size_t);
#endif
#ifndef __HAVE_ARCH_STRLCPY
size_t strlcpy(char *, const char *, size_t);
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
#ifndef __HAVE_ARCH_STRCHR
extern char * strchr(const char *,int);
#endif
#ifndef __HAVE_ARCH_STRCHRNUL
extern char * strchrnul(const char *,int);
#endif
#ifndef __HAVE_ARCH_STRRCHR
extern char * strrchr(const char *,int);
#endif
#ifndef __HAVE_ARCH_STRNCHR
extern char * strnchr(const char *, size_t, int);
#endif

extern char * __must_check skip_spaces(const char *);

extern char *strim(char *);

static inline __must_check char *strstrip(char *str)
{
	return strim(str);
}

#ifndef __HAVE_ARCH_STRLEN
extern size_t strlen(const char *);
#endif
#ifndef __HAVE_ARCH_STRNLEN
extern size_t strnlen(const char *,size_t);
#endif

#ifndef __HAVE_ARCH_STRSPN
extern size_t strspn(const char *,const char *);
#endif
#ifndef __HAVE_ARCH_STRCSPN
extern size_t strcspn(const char *,const char *);
#endif

#ifndef __HAVE_ARCH_STRPBRK
extern char * strpbrk(const char *,const char *);
#endif

#ifndef __HAVE_ARCH_STRSEP
extern char * strsep(char **,const char *);
#endif

int match_string(const char * const *array, size_t n, const char *string);

#ifndef __HAVE_ARCH_MEMSET
extern void * memset(void *,int,size_t);
#endif

void memzero_explicit(void *s, size_t count);

#ifndef __HAVE_ARCH_MEMCPY
extern void * memcpy(void *,const void *,size_t);
#endif
#ifndef __HAVE_ARCH_MEMMOVE
extern void * memmove(void *,const void *,size_t);
#endif
#ifndef __HAVE_ARCH_MEMCMP
extern int memcmp(const void *,const void *,size_t);
#endif
#ifndef __HAVE_ARCH_BCMP
extern int bcmp(const void *,const void *,size_t);
#endif
#ifndef __HAVE_ARCH_MEMSCAN
extern void * memscan(void *,int,size_t);
#endif
#ifndef __HAVE_ARCH_STRSTR
extern char * strstr(const char *, const char *);
#endif
#ifndef __HAVE_ARCH_STRNSTR
extern char * strnstr(const char *, const char *, size_t);
#endif
#ifndef __HAVE_ARCH_MEMCHR
extern void * memchr(const void *,int,size_t);
#endif

void *memchr_inv(const void *s, int c, size_t n);

char *strreplace(char *s, char old, char new);

/**
 * strstarts - does @str start with @prefix?
 * @str: string to examine
 * @prefix: prefix to look for.
 */
static inline bool strstarts(const char *str, const char *prefix)
{
	return strncmp(str, prefix, strlen(prefix)) == 0;
}

/**
 * basename - return the last part of a pathname.
 *
 * @path: path to extract the filename from.
 */
static inline const char *basename(const char *path)
{
	const char *tail = strrchr(path, '/');
	return tail ? tail + 1 : path;
}

#endif /* !__BASE_STRING_H_ */
