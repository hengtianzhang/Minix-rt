#ifndef __SEL4M_BUILD_SALT_H_
#define __SEL4M_BUILD_SALT_H_

#include <base/elfnote.h>

#define SEL4M_ELFNOTE_BUILD_SALT       0x100

#ifdef __ASSEMBLER__

#define BUILD_SALT \
       ELFNOTE(Sel4m, SEL4M_ELFNOTE_BUILD_SALT, .asciz CONFIG_BUILD_SALT)

#else

#define BUILD_SALT \
       ELFNOTE32("Sel4m", SEL4M_ELFNOTE_BUILD_SALT, CONFIG_BUILD_SALT)

#endif

#endif /* !__SEL4M_BUILD_SALT_H_ */
