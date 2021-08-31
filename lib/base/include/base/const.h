#ifndef __BASE_CONST_H_
#define __BASE_CONST_H_

#ifdef __ASSEMBLY__
#define _AC(X,Y)	X
#define _AT(T,X)	X
#else
#define __AC(X,Y)	(X##Y)
#define _AC(X,Y)	__AC(X,Y)
#define _AT(T,X)	((T)(X))
#endif

#define _ULL(x)		(_AC(x, ULL))

#define _BITULL(x)	(_ULL(1) << (x))

#define ULL(x)		(_ULL(x))

#endif /* !__BASE_CONST_H_ */
