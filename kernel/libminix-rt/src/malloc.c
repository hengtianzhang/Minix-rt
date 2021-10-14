#include <stdlib.h>

#include <base/compiler.h>
#include <base/common.h>
#include <base/errno.h>

#include <asm/base/page-def.h>

#include <memalloc/memblock.h>

#include <libminix_rt/brk.h>

static unsigned long start_base = 0;
static unsigned long current_base = 0;
static struct memblock memblock;

unsigned long get_free_page(void)
{
	void *ptr;
	unsigned long addr;

	if (!start_base) {
		memblock_init(&memblock);
		start_base = (unsigned long)sbrk(0);
	}

retry:
	addr = memblock_alloc(&memblock, PAGE_SIZE, PAGE_SIZE);
	if (!addr) {
		ptr = sbrk(PAGE_SIZE);
		if (!ptr)
			return 0;
		current_base = (unsigned long)ptr;
		memblock_add(&memblock, (unsigned long)ptr - PAGE_SIZE, PAGE_SIZE);
		goto retry;
	}

	return addr;
}

void free_page(unsigned long addr)
{
	int ret;

	BUG_ON(!IS_ALIGNED(addr, PAGE_SIZE));
	BUG_ON(addr < start_base);
	BUG_ON(addr > (current_base - PAGE_SIZE));

	memblock_free(&memblock, addr, PAGE_SIZE);
	if (current_base == (addr + PAGE_SIZE)) {
		ret = brk((void *)(current_base - PAGE_SIZE));
		BUG_ON(ret);
		memblock_remove(&memblock, addr, PAGE_SIZE);
		current_base = addr;
	}
}

struct bucket_desc {
	void *page;
	struct bucket_desc *next;
	void *freeptr;
	unsigned short refcnt;
	unsigned long bucket_size;
};

struct _bucket_dir {
	int size;
	struct bucket_desc *chain;
};

static struct _bucket_dir bucket_dir[] = {
	{ 16, NULL },	{ 32, NULL },
	{ 64, NULL },	{ 128, NULL },
	{ 256, NULL },	{ 512, NULL },
	{ 1024, NULL }, { 2048, NULL },
	{ 4096, NULL }, { 0, NULL}
};

static struct bucket_desc *free_bucket_desc = NULL;

static inline int init_bucket_desc(void)
{
	struct bucket_desc *bdesc, *first;
	int i;

	first = bdesc = (struct bucket_desc *)get_free_page();
	if (!bdesc)
		return -ENOMEM;

	for (i = (PAGE_SIZE / sizeof (struct bucket_desc)); i > 1; i--) {
		bdesc->next = bdesc + 1;
		bdesc++;
	}

	bdesc->next = free_bucket_desc;
	free_bucket_desc = first;

	return 0;
}

void *malloc(size_t size)
{
	struct _bucket_dir *bdir;
	struct bucket_desc *bdesc;
	void *retval = NULL;

	for (bdir = bucket_dir; bdir->size; bdir++)
		if (bdir->size >= size)
			break;

	if (!bdir->size)
		return retval;

	for (bdesc = bdir->chain; bdesc; bdesc = bdesc->next)
		if (bdesc->freeptr)
			break;

	if (!bdesc) {
		char *cp;
		int i;

		if (!free_bucket_desc) {
			i = init_bucket_desc();
			if (i)
				return NULL;
		}
		bdesc = free_bucket_desc;
		free_bucket_desc = bdesc->next;
		bdesc->refcnt = 0;
		bdesc->bucket_size = bdir->size;
		bdesc->page = (cp = (char *)get_free_page());
		if (!cp)
			return NULL;
		*(u16 *)cp = bdir->size;
		cp = cp + bdir->size;
		bdesc->freeptr = cp;
		for (i = PAGE_SIZE / bdir->size; i > 2; i--) {
			*((char **)cp) = cp + bdir->size;
			cp += bdir->size;
		}
		*((char **)cp) = 0;
		bdesc->next = bdir->chain;
		bdir->chain = bdesc;
	}
	retval = (void *)bdesc->freeptr;
	bdesc->freeptr = *((void **)retval);
	bdesc->refcnt++;

	return retval;
}

void free(void *addr)
{
	void *page, *freeptr;
	u16 size;
	struct _bucket_dir *bdir;
	struct bucket_desc *bdesc, *prev;

	page = (void *)((unsigned long)addr & PAGE_MASK);
	size = *(u16 *)page;

	BUG_ON((unsigned long)addr & (size - 1));

	for (bdir = bucket_dir; bdir->size; bdir++) {
		prev = NULL;

		if (bdir->size < size)
			continue;

		for (bdesc = bdir->chain; bdesc; bdesc = bdesc->next) {
			if (bdesc->page == page)
				goto found;

			prev = bdesc;
		}
	}
	BUG();

found:
	freeptr = (void *)bdesc->freeptr;
	if (freeptr != NULL)
		while (*(void **)freeptr) {
			if (*(void **)freeptr == *(void **)addr)
				BUG();
			freeptr = *(void **)freeptr;
		}

	*((void **)addr) = bdesc->freeptr;
	bdesc->freeptr = addr;
	bdesc->refcnt--;
	if (bdesc->refcnt == 0) {
		if ((prev && (prev->next != bdesc)) ||
			(!prev && (bdir->chain != bdesc)))
			for (prev = bdir->chain; prev; prev = prev->next)
				if (prev->next == bdesc)
					break;

		if (prev)
			prev->next = bdesc->next;
		else {
			if (bdir->chain != bdesc)
				hang("Malloc bucket chains corrupted\n");
			bdir->chain = bdesc->next;
		}
		free_page((unsigned long)bdesc->page);
		bdesc->next = free_bucket_desc;
		free_bucket_desc = bdesc;
	}
}
