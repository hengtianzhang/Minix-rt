#include <stdlib.h>
#include <string.h>

#include <base/compiler.h>
#include <base/common.h>
#include <base/errno.h>
#include <base/list.h>

#include <asm/base/page-def.h>

#include <memalloc/memblock.h>

#include <libminix_rt/brk.h>

static unsigned long start_base = 0;
static unsigned long current_base = 0;
static struct memblock memblock;

unsigned long get_free_pages(int nr_pages)
{
	void *ptr;
	unsigned long addr;
	unsigned long size = PAGE_SIZE * nr_pages;

	if (!size)
		return 0;

	if (!start_base) {
		memblock_init(&memblock);
		start_base = (unsigned long)sbrk(0);
	}

retry:
	addr = memblock_alloc(&memblock, size, PAGE_SIZE);
	if (!addr) {
		ptr = sbrk(size);
		if (!ptr)
			return 0;
		current_base = (unsigned long)ptr;
		memblock_add(&memblock, (unsigned long)ptr - size, size);
		goto retry;
	}

	return addr;
}

void free_pages(unsigned long addr, int nr_pages)
{
	int ret;
	u64 i;
	phys_addr_t start, end;
	unsigned long size = PAGE_SIZE * nr_pages;

	BUG_ON(!size);
	BUG_ON(!IS_ALIGNED(addr, PAGE_SIZE));
	BUG_ON(addr < start_base);
	BUG_ON(addr > (current_base - PAGE_SIZE));

	memblock_free(&memblock, addr, size);
	if (current_base == (addr + size)) {
		for_each_free_mem_range(&memblock, i, 0, &start, &end);

		ret = brk((void *)start);
		BUG_ON(ret);
		memblock_remove(&memblock, start, end);
		current_base = start;
	}
}

struct bkmap_desc {
	unsigned long start, end;
	struct list_head list;
	unsigned long ref_count;
	int *map;
};

static LIST_HEAD(bkmap_list);

static inline unsigned long calc_perpages_managed_bytes(void)
{
	return ((PAGE_SIZE - sizeof (struct bkmap_desc)) / sizeof (int)) * PAGE_SIZE;
}

static struct bkmap_desc *init_bkmap_desc(unsigned long addr)
{
	int i;
	struct bkmap_desc *bkmap;
	unsigned long managed_bytes, size;
	unsigned long start;

	bkmap = (struct bkmap_desc *)get_free_pages(1);
	if (!bkmap)
		return NULL;

	memset(bkmap, 0, PAGE_SIZE);
	managed_bytes = calc_perpages_managed_bytes();
	size = addr - start_base;
	i = size / managed_bytes;
	start = start_base + (i * managed_bytes);

	bkmap->ref_count = 0;
	bkmap->start = start;
	bkmap->end = start + managed_bytes;
	bkmap->map = (void *)bkmap + sizeof (struct bkmap_desc);
	list_add(&bkmap->list, &bkmap_list);

	return bkmap;
}

static struct bkmap_desc *bkmap_find_area(unsigned long addr)
{
	struct bkmap_desc *bkmap = NULL;

	list_for_each_entry(bkmap, &bkmap_list, list) {
		if ((bkmap->start <= addr) && (addr < bkmap->end))
			goto found;
	}

	bkmap = init_bkmap_desc(addr);
found:
	return bkmap;
}

static int bkmap_set_page_size(unsigned long addr, unsigned long size)
{
	struct bkmap_desc *bkmap;
	int index;

	bkmap = bkmap_find_area(addr);
	if (!bkmap)
		return -ENOMEM;

	index = (addr - bkmap->start) >> PAGE_SHIFT;
	if (index < 0)
		return -EINVAL;

	if (bkmap->map[index] != 0)
		return -EINVAL;

	bkmap->map[index] = size;
	bkmap->ref_count++;

	return 0;
}

static unsigned long bkmap_get_page_size(unsigned long addr)
{
	struct bkmap_desc *bkmap;
	int index;

	bkmap = bkmap_find_area(addr);
	if (!bkmap)
		return 0;

	index = (addr - bkmap->start) >> PAGE_SHIFT;
	if (index < 0)
		return 0;

	return bkmap->map[index];
}

static unsigned long bkmap_put_page_size(unsigned long addr)
{
	struct bkmap_desc *bkmap;
	unsigned long size;
	int index;

	bkmap = bkmap_find_area(addr);
	if (!bkmap)
		return 0;

	index = (addr - bkmap->start) >> PAGE_SHIFT;
	if (index < 0)
		return 0;

	if (bkmap->map[index] == 0)
		return 0;

	size = bkmap->map[index];
	bkmap->map[index] = 0;
	bkmap->ref_count--;
	if (bkmap->ref_count == 0) {
		list_del(&bkmap->list);
		free_pages((unsigned long)bkmap, 1);
	}

	return size;
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
	{ 0, NULL}
};

struct bdescmap_desc {
	int ref_count;
	struct bucket_desc *freeptr;
	struct list_head list;
	struct bucket_desc *map;
};

static LIST_HEAD(bdescmap_list);

static struct bdescmap_desc *init_bdescmap_desc(void)
{
	struct bdescmap_desc *bdescmap;
	struct bucket_desc *first;
	int i;

	bdescmap = (struct bdescmap_desc *)get_free_pages(1);
	if (!bdescmap)
		return NULL;

	memset(bdescmap, 0, PAGE_SIZE);
	bdescmap->ref_count = 0;
	bdescmap->map = (void *)bdescmap +
				sizeof (struct bdescmap_desc);
	first = bdescmap->map;
	bdescmap->freeptr = first;

	for (i = ((PAGE_SIZE - sizeof (struct bdescmap_desc)) / sizeof (struct bucket_desc));
			i > 1; i--) {
		first->next = first + 1;
		first++;
	}
	first->next = NULL;
	list_add(&bdescmap->list, &bdescmap_list);

	return bdescmap;
}

static struct bucket_desc *bdescmap_get_desc(void)
{
	struct bdescmap_desc *bdescmap;
	struct bucket_desc *bdesc;

	list_for_each_entry(bdescmap, &bdescmap_list, list) {
		if (bdescmap->freeptr)
			goto found;
	}

	bdescmap = init_bdescmap_desc();
found:
	bdescmap->ref_count++;
	bdesc = bdescmap->freeptr;
	bdescmap->freeptr = bdesc->next;
	bdesc->next = NULL;

	return bdesc;
}

static int bdescmap_put_bdesc(struct bucket_desc *bdesc)
{
	struct bdescmap_desc *bdescmap;
	struct bucket_desc *freeptr;

	list_for_each_entry(bdescmap, &bdescmap_list, list) {
		if (((unsigned long)bdescmap < (unsigned long)bdesc) &&
			((unsigned long)bdescmap + PAGE_SIZE) > (unsigned long)bdesc)
			goto found;
	}

	return -EINVAL;

found:
	freeptr = bdescmap->freeptr;
	bdesc->next = freeptr;
	bdescmap->freeptr = bdesc;
	bdescmap->ref_count--;
	if (bdescmap->ref_count == 0) {
		list_del(&bdescmap->list);
		free_pages((unsigned long)bdescmap, 1);
	}
	return 0;
}

static inline int nr_pages_size(size_t size)
{
	return ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT;
}

void *malloc(size_t size)
{
	int nr_pages, ret;
	unsigned long addr;
	struct _bucket_dir *bdir;
	struct bucket_desc *bdesc;
	void *retval = NULL;

	if (size > PAGE_SIZE / 2) {
		nr_pages = nr_pages_size(size);
		addr = get_free_pages(nr_pages);
		if (!addr)
			return NULL;

		ret = bkmap_set_page_size(addr, size);
		if (ret) {
			free_pages(addr, nr_pages);
			return NULL;
		}
		return (void *)addr;
	}

	for (bdir = bucket_dir; bdir->size; bdir++)
		if (bdir->size >= size)
			break;

	if (!bdir->size)
		return NULL;

	for (bdesc = bdir->chain; bdesc; bdesc = bdesc->next)
		if (bdesc->freeptr)
			break;

	if (!bdesc) {
		char *cp;
		int i;

		bdesc = bdescmap_get_desc();
		if (!bdesc)
			return NULL;

		cp = (char *)get_free_pages(1);
		if (!cp) {
			i = bdescmap_put_bdesc(bdesc);
			BUG_ON(i);

			return NULL;
		}
		i = bkmap_set_page_size((unsigned long)cp, bdir->size);
		if (i) {
			i = bdescmap_put_bdesc(bdesc);
			BUG_ON(i);

			free_pages((unsigned long)cp, 1);

			return NULL;
		}
		bdesc->refcnt = 0;
		bdesc->bucket_size = bdir->size;
		bdesc->page = cp;
		bdesc->freeptr = cp;
		for (i = PAGE_SIZE / bdir->size; i > 1; i--) {
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

void *zalloc(size_t size)
{
	void *addr;

	addr = malloc(size);
	if (addr) {
		memset(addr, 0, size);
	}
	return addr;
}

void free(void *addr)
{
	int nr_pages, ret;
	unsigned long size;
	void *page, *freeptr;
	struct _bucket_dir *bdir;
	struct bucket_desc *bdesc, *prev;

	page = (void *)((unsigned long)addr & PAGE_MASK);
	size = bkmap_get_page_size((unsigned long)page);
	BUG_ON(!size);

	if (size > PAGE_SIZE / 2) {
		BUG_ON(!ALIGN(size, PAGE_SIZE));
		BUG_ON(!ALIGN((unsigned long)addr, PAGE_SIZE));

		size = bkmap_put_page_size((unsigned long)addr);
		BUG_ON(!size);
		nr_pages = nr_pages_size(size);
		free_pages((unsigned long)addr, nr_pages);
		return ;
	}

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
			if (*(void **)freeptr == addr)
				BUG();
			freeptr = *(void **)freeptr;
		}

	*((void **)addr) = bdesc->freeptr;
	bdesc->freeptr = addr;
	bdesc->refcnt--;
	if (bdesc->refcnt == 0) {
		if (!prev)
			bdir->chain = bdesc->next;
		else
			prev->next = bdesc->next;
		ret = bdescmap_put_bdesc(bdesc);
		BUG_ON(ret);
		size = bkmap_put_page_size((unsigned long)page);
		BUG_ON(!size);
		free_pages((unsigned long)bdesc->page, 1);
	}
}
