#include <stdlib.h>

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
	unsigned long size = PAGE_SIZE * nr_pages;

	BUG_ON(!size);
	BUG_ON(!IS_ALIGNED(addr, PAGE_SIZE));
	BUG_ON(addr < start_base);
	BUG_ON(addr > (current_base - PAGE_SIZE));

	memblock_free(&memblock, addr, size);
	if (current_base == (addr + size)) {
		ret = brk((void *)(current_base - size));
		BUG_ON(ret);
		memblock_remove(&memblock, addr, size);
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
	{ 0, NULL}
};

static struct bucket_desc *free_bucket_desc = NULL;

struct bdesc_store_desc {
	int ref_count;
	struct list_head list;
	struct bucket_desc *bdesc_map;
};

static LIST_HEAD(bdesc_store_list);

static struct bdesc_store_desc *init_bdesc_store_map(void)
{
	struct bdesc_store_desc *bd_store_map;
	struct bucket_desc *first;
	int i;

	bd_store_map = (struct bdesc_store_desc *)get_free_pages(1);
	if (!bd_store_map)
		return NULL;

	bd_store_map->ref_count = 0;
	bd_store_map->bdesc_map = (void *)bd_store_map +
					sizeof (struct bdesc_store_desc);
	first = bd_store_map->bdesc_map;
	
	for (i = ((PAGE_SIZE - sizeof (struct bdesc_store_desc)) / sizeof (struct bucket_desc));
			i > 1; i--) {
		first->page = NULL;
		first->next = first + 1;
		first++;
	}

	first->page = NULL;
	first->next = NULL;
	list_add(&bd_store_map->list, &bdesc_store_list);

	return bd_store_map;
}

static struct bdesc_store_desc *bdesc_store_find_area(struct bucket_desc *bdesc)
{
	struct bdesc_store_desc *bd_store_map = NULL;

	list_for_each_entry(bd_store_map, &bdesc_store_list, list) {
		if (((unsigned long)bd_store_map < (unsigned long)bdesc) &&
			((unsigned long)bd_store_map + PAGE_SIZE) > (unsigned long)bdesc)
			return bd_store_map;
	}

	return NULL;
}

static struct bdesc_store_desc *init_bucket_desc(void)
{
	struct bdesc_store_desc *bd_store_map;

	bd_store_map = bdesc_store_find_area(free_bucket_desc);
	if (!bd_store_map) {
		bd_store_map = init_bdesc_store_map();
		if (!bd_store_map)
			return NULL;
		free_bucket_desc = bd_store_map->bdesc_map;
	}

	return bd_store_map;
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
	return ((PAGE_SIZE - sizeof (struct bkmap_desc))/sizeof (int)) * PAGE_SIZE;
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

	managed_bytes = calc_perpages_managed_bytes();
	size = addr - start_base;
	i = size / managed_bytes;
	start = start_base + (i * managed_bytes);

	bkmap->ref_count = 0;
	bkmap->start = start;
	bkmap->end = start + managed_bytes;
	bkmap->map = (int *)bkmap + sizeof (struct bkmap_desc);
	list_add(&bkmap->list, &bkmap_list);

	return bkmap;
}

static struct bkmap_desc *bkmap_find_area(unsigned long addr)
{
	struct bkmap_desc *bkmap = NULL;

	list_for_each_entry(bkmap, &bkmap_list, list) {
		if ((bkmap->start <= addr) && (addr < bkmap->end))
			return bkmap;
	}

	return NULL;
}

static inline int nr_pages_size(size_t size)
{
	return ALIGN(size, PAGE_SIZE) >> PAGE_SHIFT;
}

void *malloc(size_t size)
{
	int nr_pages;
	struct _bucket_dir *bdir;
	struct bucket_desc *bdesc;
	struct bkmap_desc *bkmap = NULL;
	void *retval = NULL;

	if (size > PAGE_SIZE / 2) {
		unsigned long addr;

		nr_pages = nr_pages_size(size);
		addr = get_free_pages(nr_pages);
		if (!addr)
			return NULL;

		bkmap = bkmap_find_area(addr);
		if (!bkmap)
			bkmap = init_bkmap_desc(addr);
		if (!bkmap) {
			free_pages(addr, nr_pages);
			return NULL;
		}

		bkmap->ref_count++;
		bkmap->map[(addr - bkmap->start) >> PAGE_SHIFT] = nr_pages * PAGE_SIZE;

		return (void *)addr;
	}

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
		struct bdesc_store_desc *bd_store_map;

		bd_store_map = bdesc_store_find_area(free_bucket_desc);
		if (!bd_store_map) {
			bd_store_map = init_bucket_desc();
			if (!bd_store_map)
				return NULL;
		}
		bdesc = free_bucket_desc;
		free_bucket_desc = bdesc->next;
		bdesc->refcnt = 0;
		bdesc->bucket_size = bdir->size;
		cp = (char *)get_free_pages(1);
		if (!cp)
			return NULL;

		bkmap = bkmap_find_area((unsigned long)cp);
		if (!bkmap)
			bkmap = init_bkmap_desc((unsigned long)cp);
		if (!bkmap) {
			free_pages((unsigned long)cp, 1);
			return NULL;
		}

		bkmap->ref_count++;
		bkmap->map[((unsigned long)cp - bkmap->start) >> PAGE_SHIFT] = bdir->size;

		bdesc->page = cp;
		bdesc->freeptr = cp;
		for (i = PAGE_SIZE / bdir->size; i > 1; i--) {
			*((char **)cp) = cp + bdir->size;
			cp += bdir->size;
		}
		*((char **)cp) = 0;
		bdesc->next = bdir->chain;
		bdir->chain = bdesc;
		bd_store_map->ref_count++;
	}
	retval = (void *)bdesc->freeptr;
	bdesc->freeptr = *((void **)retval);
	bdesc->refcnt++;

	return retval;
}

void free(void *addr)
{
	void *page, *freeptr;
	size_t size;
	int nr_pages;
	struct _bucket_dir *bdir;
	struct bucket_desc *bdesc, *prev;
	struct bkmap_desc *bkmap = NULL;

	page = (void *)((unsigned long)addr & PAGE_MASK);
	bkmap = bkmap_find_area((unsigned long)page);
	BUG_ON(!bkmap);

	size = bkmap->map[((unsigned long)page - bkmap->start) >> PAGE_SHIFT];

	if (size > PAGE_SIZE / 2) {
		BUG_ON(!ALIGN(size, PAGE_SIZE));
		BUG_ON(!ALIGN((unsigned long)addr, PAGE_SIZE));
		nr_pages = nr_pages_size(size);
		free_pages((unsigned long)page, nr_pages);
		bkmap->ref_count--;
		if (bkmap->ref_count == 0) {
			free_pages((unsigned long)bkmap, 1);
			list_del(&bkmap->list);
		}
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
			if (*(void **)freeptr == *(void **)addr)
				BUG();
			freeptr = *(void **)freeptr;
		}

	*((void **)addr) = bdesc->freeptr;
	bdesc->freeptr = addr;
	bdesc->refcnt--;
	if (bdesc->refcnt == 0) {
		struct bdesc_store_desc *bd_store_map;

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
		free_pages((unsigned long)bdesc->page, 1);
		bdesc->page = NULL;
		bdesc->next = free_bucket_desc;
		free_bucket_desc = bdesc;
		bd_store_map = bdesc_store_find_area(free_bucket_desc);
		BUG_ON(!bd_store_map);
		bd_store_map->ref_count--;
		bkmap->ref_count--;
		if (bd_store_map->ref_count == 0) {
			list_del(&bd_store_map->list);
			free_pages((unsigned long)bd_store_map, 1);
		}
		if (bkmap->ref_count == 0) {
			list_del(&bkmap->list);
			free_pages((unsigned long)bkmap, 1);	
		}
	}
}
