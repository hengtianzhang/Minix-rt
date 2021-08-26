#ifndef __SEL4M_INIT_H_
#define __SEL4M_INIT_H_

#include <sel4m/compiler.h>

#define __init		__section(.init.text) __cold
#define __initdata	__section(.init.data)
#define __initconst __section(.init.rodata)

#define __HEAD		.section	".head.text","ax"
#define __INIT		.section	".init.text","ax"
#define __FINIT		.previous

#define __INITDATA	.section	".init.data","aw",%progbits
#define __INITRODATA	.section	".init.rodata","a",%progbits
#define __FINITDATA	.previous

#endif
