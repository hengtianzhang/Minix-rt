#ifndef __MINIX_RT_BUILD_SALT_H_
#define __MINIX_RT_BUILD_SALT_H_

#include <base/elfnote.h>

#define MINIX_RT_ELFNOTE_BUILD_SALT       0x100

#ifdef __ASSEMBLER__

#define BUILD_SALT \
       ELFNOTE(Minix-rt, MINIX_RT_ELFNOTE_BUILD_SALT, .asciz CONFIG_BUILD_SALT)

#else

#define BUILD_SALT \
       ELFNOTE32("Minix-rt", MINIX_RT_ELFNOTE_BUILD_SALT, CONFIG_BUILD_SALT)

#endif

#endif /* !__MINIX_RT_BUILD_SALT_H_ */
