#ifndef __MEMALLOC_MEMBLOCK_H_
#define __MEMALLOC_MEMBLOCK_H_

enum memblock_flags {
	MEMBLOCK_NONE       = 0x0, /* No special request */
	MEMBLOCK_NOMAP		= 0x1,
	MEMBLOCK_DMA		= 0x2,
	MEMBLOCK_MOVABLE	= 0x3,
};

#endif /* !__MEMALLOC_MEMBLOCK_H_ */
