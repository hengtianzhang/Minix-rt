/*
 * Copyright (C) 2013 ARM Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef __ASM_MISC_STRING_H_
#define __ASM_MISC_STRING_H_

#define __HAVE_ARCH_STRRCHR
extern char *strrchr(const char *, int c);

#define __HAVE_ARCH_STRCHR
extern char *strchr(const char *, int c);

#define __HAVE_ARCH_STRCMP
extern int strcmp(const char *, const char *);

#define __HAVE_ARCH_STRNCMP
extern int strncmp(const char *, const char *, size_t);

#define __HAVE_ARCH_STRLEN
extern size_t strlen(const char *);

#define __HAVE_ARCH_STRNLEN
extern size_t strnlen(const char *, size_t);

#define __HAVE_ARCH_MEMCMP
extern int memcmp(const void *, const void *, size_t);

#define __HAVE_ARCH_MEMCHR
extern void *memchr(const void *, int, size_t);

#define __HAVE_ARCH_MEMCPY
extern void *memcpy(void *, const void *, size_t);
extern void *__memcpy(void *, const void *, size_t);

#define __HAVE_ARCH_MEMMOVE
extern void *memmove(void *, const void *, size_t);
extern void *__memmove(void *, const void *, size_t);

#define __HAVE_ARCH_MEMSET
extern void *memset(void *, int, size_t);
extern void *__memset(void *, int, size_t);

#endif /* !__ASM_MISC_STRING_H_ */
