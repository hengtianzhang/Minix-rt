#ifndef __BASE_INIT_H_
#define __BASE_INIT_H_

#include <base/compiler.h>

#ifdef __KERNEL__

#define __init		__section(.init.text) __cold
#define __initdata	__section(.init.data)
#define __initconst __section(.init.rodata)

#define __HEAD		.section	".head.text","ax"
#define __INIT		.section	".init.text","ax"
#define __FINIT		.previous

#define __INITDATA	.section	".init.data","aw",%progbits
#define __INITRODATA	.section	".init.rodata","a",%progbits
#define __FINITDATA	.previous

#else

#define __init
#define __initdata
#define __initconst

#define __HEAD
#define __INIT
#define __FINIT

#define __INITDATA
#define __INITRODATA
#define __FINITDATA

#endif /* __KERNEL__ */
#endif /* !__BASE_INIT_H_ */
