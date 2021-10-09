/*
 * SLUB: A slab allocator that limits cache line use instead of queuing
 * objects in per cpu and per node lists.
 *
 * The allocator synchronizes using per slab locks and only
 * uses a centralized lock to manage a pool of partial slabs.
 *
 * (C) 2007 SGI, Christoph Lameter <clameter@sgi.com>
 */
#include <base/cache.h>
#include <base/poison.h>

#include <minix_rt/smp.h>
#include <minix_rt/bit_spinlock.h>
#include <minix_rt/slab.h>
#include <minix_rt/mutex.h>
#include <minix_rt/cpumask.h>

#define FROZEN (1 << PG_active)

static inline int SlabFrozen(struct page *page)
{
	return page->flags & FROZEN;
}

static inline void SetSlabFrozen(struct page *page)
{
	page->flags |= FROZEN;
}

static inline void ClearSlabFrozen(struct page *page)
{
	page->flags &= ~FROZEN;
}

#if PAGE_SHIFT <= 12

/*
 * Small page size. Make sure that we do not fragment memory
 */
#define DEFAULT_MAX_ORDER 1
#define DEFAULT_MIN_OBJECTS 4

#else

/*
 * Large page machines are customarily able to handle larger
 * page orders.
 */
#define DEFAULT_MAX_ORDER 2
#define DEFAULT_MIN_OBJECTS 8

#endif

/*
 * Mininum number of partial slabs. These will be left on the partial
 * lists even if they are empty. kmem_cache_shrink may reclaim them.
 */
#define MIN_PARTIAL 5

/*
 * Maximum number of desirable partial slabs.
 * The existence of more partial slabs makes kmem_cache_shrink
 * sort the partial list by the number of objects in the.
 */
#define MAX_PARTIAL 10

/*
 * Set of flags that will prevent slab merging
 */
#define SLUB_NEVER_MERGE (SLAB_RED_ZONE | SLAB_POISON | SLAB_STORE_USER)

#define SLUB_MERGE_SAME (SLAB_CACHE_DMA)

/* Internal SLUB flags */
#define __OBJECT_POISON		0x80000000 /* Poison object */

static int kmem_size = sizeof(struct kmem_cache);

static enum {
	DOWN,		/* No slab functionality available */
	PARTIAL,	/* kmem_cache_open() works but kmalloc does not */
	UP,		/* Everything works but does not show up in sysfs */
} slab_state = DOWN;

int slab_is_available(void)
{
	return slab_state >= UP;
}

static LIST_HEAD(slab_caches);
static DEFINE_MUTEX(slub_lock);

static inline struct kmem_cache_node *get_node(struct kmem_cache *s)
{
	return &s->local_node;
}

static inline struct kmem_cache_cpu *get_cpu_slab(struct kmem_cache *s, int cpu)
{
	return s->cpu_slab[cpu];
}

static inline int check_valid_pointer(struct kmem_cache *s,
				struct page *page, const void *object)
{
	void *base;

	if (!object)
		return 1;

	base = page_address(page);
	if (object < base || object >= base + s->objects * s->size ||
		(object - base) % s->size) {
		return 0;
	}

	return 1;
}

/*
 * Slow version of get and set free pointer.
 *
 * This version requires touching the cache lines of kmem_cache which
 * we avoid to do in the fast alloc free paths. There we obtain the offset
 * from the page struct.
 */
static inline void *get_freepointer(struct kmem_cache *s, void *object)
{
	return *(void **)(object + s->offset);
}

static inline void set_freepointer(struct kmem_cache *s, void *object, void *fp)
{
	*(void **)(object + s->offset) = fp;
}

/* Loop over all objects in a slab */
#define for_each_object(__p, __s, __addr) \
	for (__p = (__addr); __p < (__addr) + (__s)->objects * (__s)->size;\
			__p += (__s)->size)

/* Scan freelist */
#define for_each_free_object(__p, __s, __free) \
	for (__p = (__free); __p; __p = get_freepointer((__s), __p))

/* Determine object index from a given position */
static inline int slab_index(void *p, struct kmem_cache *s, void *addr)
{
	return (p - addr) / s->size;
}

/*
 * Slab allocation and freeing
 */
static struct page *allocate_slab(struct kmem_cache *s, gfp_t flags)
{
	struct page * page;

	if (s->flags & SLAB_CACHE_DMA)
		flags |= GFP_DMA;

	page = alloc_pages(flags, s->order);
	if (!page)
		return NULL;

	return page;
}

static void setup_object(struct kmem_cache *s, struct page *page,
				void *object)
{
	if (unlikely(s->ctor))
		s->ctor(s, object);
}

static struct page *new_slab(struct kmem_cache *s, gfp_t flags)
{
	struct page *page;
	struct kmem_cache_node *n;
	void *start;
	void *last;
	void *p;

	page = allocate_slab(s, flags);
	if (!page)
		goto out;

	n = get_node(s);
	if (n)
		atomic_long_inc(&n->nr_slabs);
	page->slab = s;
	__SetPageSlub(page);

	start = page_address(page);

	if (unlikely(s->flags & SLAB_POISON))
		memset(start, POISON_INUSE, PAGE_SIZE << s->order);

	last = start;
	for_each_object(p, s, start) {
		setup_object(s, page, last);
		set_freepointer(s, last, p);
		last = p;
	}
	setup_object(s, page, last);
	set_freepointer(s, last, NULL);

	page->freelist = start;
	page->inuse = 0;
out:
	return page;
}

static void __free_slab(struct kmem_cache *s, struct page *page)
{
	__free_pages(page, s->order);
}

static void free_slab(struct kmem_cache *s, struct page *page)
{
	__free_slab(s, page);
}

static void discard_slab(struct kmem_cache *s, struct page *page)
{
	struct kmem_cache_node *n = get_node(s);

	atomic_long_dec(&n->nr_slabs);
	page_mapcount_reset(page);
	__ClearPageSlub(page);
	free_slab(s, page);
}

/*
 * Per slab locking using the pagelock
 */
static __always_inline void slab_lock(struct page *page)
{
	BUG_ON(PageTail(page));
	bit_spin_lock(PG_locked, &page->flags);
}

static __always_inline void slab_unlock(struct page *page)
{
	BUG_ON(PageTail(page));
	__bit_spin_unlock(PG_locked, &page->flags);
}

static __always_inline int slab_trylock(struct page *page)
{
	int rc = 1;

	rc = bit_spin_trylock(PG_locked, &page->flags);
	return rc;
}

/*
 * Management of partially allocated slabs
 */
static void add_partial_tail(struct kmem_cache_node *n, struct page *page)
{
	spin_lock(&n->list_lock);
	n->nr_partial++;
	list_add_tail(&page->lru, &n->partial);
	spin_unlock(&n->list_lock);
}

static void add_partial(struct kmem_cache_node *n, struct page *page)
{
	spin_lock(&n->list_lock);
	n->nr_partial++;
	list_add(&page->lru, &n->partial);
	spin_unlock(&n->list_lock);
}

static void remove_partial(struct kmem_cache *s,
						struct page *page)
{
	struct kmem_cache_node *n = get_node(s);

	spin_lock(&n->list_lock);
	list_del(&page->lru);
	n->nr_partial--;
	spin_unlock(&n->list_lock);
}

/*
 * Lock slab and remove from the partial list.
 *
 * Must hold list_lock.
 */
static inline int lock_and_freeze_slab(struct kmem_cache_node *n, struct page *page)
{
	if (slab_trylock(page)) {
		list_del(&page->lru);
		n->nr_partial--;
		SetSlabFrozen(page);
		return 1;
	}
	return 0;
}

/*
 * Try to allocate a partial slab from a specific node.
 */
static struct page *get_partial_node(struct kmem_cache_node *n)
{
	struct page *page;

	/*
	 * Racy check. If we mistakenly see no partial slabs then we
	 * just allocate an empty slab. If we mistakenly try to get a
	 * partial slab and there is none available then get_partials()
	 * will return NULL.
	 */
	if (!n || !n->nr_partial)
		return NULL;

	spin_lock(&n->list_lock);
	list_for_each_entry(page, &n->partial, lru)
		if (lock_and_freeze_slab(n, page))
			goto out;
	page = NULL;
out:
	spin_unlock(&n->list_lock);
	return page;
}

/*
 * Get a partial page, lock it and return it.
 */
static struct page *get_partial(struct kmem_cache *s, gfp_t flags)
{
	return get_partial_node(get_node(s));
}

/*
 * Move a page back to the lists.
 *
 * Must be called with the slab lock held.
 *
 * On exit the slab lock will have been dropped.
 */
static void unfreeze_slab(struct kmem_cache *s, struct page *page)
{
	struct kmem_cache_node *n = get_node(s);

	ClearSlabFrozen(page);
	if (page->inuse) {

		if (page->freelist)
			add_partial(n, page);
		slab_unlock(page);
	} else {
		if (n->nr_partial < MIN_PARTIAL) {
			/*
			 * Adding an empty slab to the partial slabs in order
			 * to avoid page allocator overhead. This slab needs
			 * to come after the other slabs with objects in
			 * order to fill them up. That way the size of the
			 * partial list stays small. kmem_cache_shrink can
			 * reclaim empty slabs from the partial list.
			 */
			add_partial_tail(n, page);
			slab_unlock(page);
		} else {
			slab_unlock(page);
			discard_slab(s, page);
		}
	}
}

/*
 * Remove the cpu slab
 */
static void deactivate_slab(struct kmem_cache *s, struct kmem_cache_cpu *c)
{
	struct page *page = c->page;
	/*
	 * Merge cpu freelist into freelist. Typically we get here
	 * because both freelists are empty. So this is unlikely
	 * to occur.
	 */
	while (unlikely(c->freelist)) {
		void **object;

		/* Retrieve object from cpu_freelist */
		object = c->freelist;
		c->freelist = c->freelist[c->offset];

		/* And put onto the regular freelist */
		object[c->offset] = page->freelist;
		page->freelist = object;
		page->inuse--;
	}
	c->page = NULL;
	unfreeze_slab(s, page);
}

static inline void flush_slab(struct kmem_cache *s, struct kmem_cache_cpu *c)
{
	slab_lock(c->page);
	deactivate_slab(s, c);
}

/*
 * Flush cpu slab.
 * Called from IPI handler with interrupts disabled.
 */
static inline void __flush_cpu_slab(struct kmem_cache *s, int cpu)
{
	struct kmem_cache_cpu *c = get_cpu_slab(s, cpu);

	if (likely(c && c->page))
		flush_slab(s, c);
}

static void flush_cpu_slab(void *d)
{
	struct kmem_cache *s = d;

	__flush_cpu_slab(s, smp_processor_id());
}

static void flush_all(struct kmem_cache *s)
{
	on_each_cpu(flush_cpu_slab, s, 1);
}

/*
 * Slow path. The lockless freelist is empty or we need to perform
 * debugging duties.
 *
 * Interrupts are disabled.
 *
 * Processing is still very fast if new objects have been freed to the
 * regular freelist. In that case we simply take over the regular freelist
 * as the lockless freelist and zap the regular freelist.
 *
 * If that is not working then we fall back to the partial lists. We take the
 * first element of the freelist as the object to allocate now and move the
 * rest of the freelist to the lockless freelist.
 *
 * And if we were unable to get a new slab from the partial slab lists then
 * we need to allocate a new slab. This is slowest path since we may sleep.
 */
static void *__slab_alloc(struct kmem_cache *s,
		gfp_t gfpflags, void *addr, struct kmem_cache_cpu *c)
{
	void **object;
	struct page *new;

	if (!c->page)
		goto new_slab;

	slab_lock(c->page);

load_freelist:
	object = c->page->freelist;
	if (unlikely(!object))
		goto another_slab;

	object = c->page->freelist;
	c->freelist = object[c->offset];
	c->page->inuse = s->objects;
	c->page->freelist = NULL;
	slab_unlock(c->page);
	return object;

another_slab:
	deactivate_slab(s, c);

new_slab:
	new = get_partial(s, gfpflags);
	if (new) {
		c->page = new;
		goto load_freelist;
	}

	new = new_slab(s, gfpflags);

	if (new) {
		c = get_cpu_slab(s, smp_processor_id());
		if (c->page)
			flush_slab(s, c);
		slab_lock(new);
		SetSlabFrozen(new);
		c->page = new;
		goto load_freelist;
	}
	return NULL;
}

/*
 * Inlined fastpath so that allocation functions (kmalloc, kmem_cache_alloc)
 * have the fastpath folded into their functions. So no function call
 * overhead for requests that can be satisfied on the fastpath.
 *
 * The fastpath works by first checking if the lockless freelist can be used.
 * If not then __slab_alloc is called for slow processing.
 *
 * Otherwise we can simply pick the next object from the lockless free list.
 */
static void __always_inline *slab_alloc(struct kmem_cache *s,
		gfp_t gfpflags, void *addr)
{
	void **object;
	u64 flags;
	struct kmem_cache_cpu *c;

	local_irq_save(flags);
	c = get_cpu_slab(s, smp_processor_id());
	if (unlikely(!c->freelist))
		object = __slab_alloc(s, gfpflags, addr, c);
	else {
		object = c->freelist;
		c->freelist = object[c->offset];
	}
	local_irq_restore(flags);

	if (unlikely((gfpflags & __GFP_ZERO) && object))
		memset(object, 0, c->objsize);

	return object;
}

void *kmem_cache_alloc(struct kmem_cache *s, gfp_t gfpflags)
{
	return slab_alloc(s, gfpflags, __builtin_return_address(0));
}

/*
 * Slow patch handling. This may still be called frequently since objects
 * have a longer lifetime than the cpu slabs in most processing loads.
 *
 * So we still attempt to reduce cache line usage. Just take the slab
 * lock and free the item. If there is no additional partial page
 * handling required then we can return immediately.
 */
static void __slab_free(struct kmem_cache *s, struct page *page,
				void *x, void *addr, unsigned int offset)
{
	void *prior;
	void **object = (void *)x;

	slab_lock(page);

	prior = object[offset] = page->freelist;
	page->freelist = object;
	page->inuse--;

	if (unlikely(SlabFrozen(page)))
		goto out_unlock;

	if (unlikely(!page->inuse))
		goto slab_empty;

	/*
	 * Objects left in the slab. If it
	 * was not on the partial list before
	 * then add it.
	 */
	if (unlikely(!prior))
		add_partial_tail(get_node(s), page);

out_unlock:
	slab_unlock(page);
	return;

slab_empty:
	if (prior)
		/*
		 * Slab still on the partial list.
		 */
		remove_partial(s, page);

	slab_unlock(page);
	discard_slab(s, page);
}

/*
 * Fastpath with forced inlining to produce a kfree and kmem_cache_free that
 * can perform fastpath freeing without additional function calls.
 *
 * The fastpath is only possible if we are freeing to the current cpu slab
 * of this processor. This typically the case if we have just allocated
 * the item before.
 *
 * If fastpath is not possible then fall back to __slab_free where we deal
 * with all sorts of special processing.
 */
static void __always_inline slab_free(struct kmem_cache *s,
			struct page *page, void *x, void *addr)
{
	void **object = (void *)x;
	u64 flags;
	struct kmem_cache_cpu *c;

	local_irq_save(flags);

	c = get_cpu_slab(s, smp_processor_id());
	if (likely(page == c->page)) {
		object[c->offset] = c->freelist;
		c->freelist = object;
	} else
		__slab_free(s, page, x, addr, c->offset);

	local_irq_restore(flags);
}

void kmem_cache_free(struct kmem_cache *s, void *x)
{
	struct page *page;

	page = virt_to_head_page(x);

	slab_free(s, page, x, __builtin_return_address(0));
}

/* Figure out on which slab object the object resides */
static struct page *get_object_page(const void *x)
{
	struct page *page = virt_to_head_page(x);

	if (!PageSlub(page))
		return NULL;

	return page;
}

/*
 * Object placement in a slab is made very easy because we always start at
 * offset 0. If we tune the size of the object to the alignment then we can
 * get the required alignment by putting one properly sized object after
 * another.
 *
 * Notice that the allocation order determines the sizes of the per cpu
 * caches. Each processor has always one slab available for allocations.
 * Increasing the allocation order reduces the number of times that slabs
 * must be moved on and off the partial lists and is therefore a factor in
 * locking overhead.
 */

/*
 * Mininum / Maximum order of slab pages. This influences locking overhead
 * and slab fragmentation. A higher order reduces the number of partial slabs
 * and increases the number of allocations possible without having to
 * take the list_lock.
 */
static int slub_min_order;
static int slub_max_order = DEFAULT_MAX_ORDER;
static int slub_min_objects = DEFAULT_MIN_OBJECTS;

/*
 * Calculate the order of allocation given an slab object size.
 *
 * The order of allocation has significant impact on performance and other
 * system components. Generally order 0 allocations should be preferred since
 * order 0 does not cause fragmentation in the page allocator. Larger objects
 * be problematic to put into order 0 slabs because there may be too much
 * unused space left. We go to a higher order if more than 1/8th of the slab
 * would be wasted.
 *
 * In order to reach satisfactory performance we must ensure that a minimum
 * number of objects is in one slab. Otherwise we may generate too much
 * activity on the partial lists which requires taking the list_lock. This is
 * less a concern for large slabs though which are rarely used.
 *
 * slub_max_order specifies the order where we begin to stop considering the
 * number of objects in a slab as critical. If we reach slub_max_order then
 * we try to keep the page order as low as possible. So we accept more waste
 * of space in favor of a small page order.
 *
 * Higher order allocations also allow the placement of more objects in a
 * slab and thereby reduce object handling overhead. If the user has
 * requested a higher mininum order then we start with that one instead of
 * the smallest order which will fit the object.
 */
static inline int slab_order(int size, int min_objects,
				int max_order, int fract_leftover)
{
	int order;
	int rem;
	int min_order = slub_min_order;

	for (order = max(min_order,
				fls(min_objects * size - 1) - PAGE_SHIFT);
			order <= max_order; order++) {

		unsigned long slab_size = PAGE_SIZE << order;

		if (slab_size < min_objects * size)
			continue;

		rem = slab_size % size;

		if (rem <= slab_size / fract_leftover)
			break;

	}

	return order;
}

static inline int calculate_order(int size)
{
	int order;
	int min_objects;
	int fraction;

	/*
	 * Attempt to find best configuration for a slab. This
	 * works by first attempting to generate a layout with
	 * the best configuration and backing off gradually.
	 *
	 * First we reduce the acceptable waste in a slab. Then
	 * we reduce the minimum objects required in a slab.
	 */
	min_objects = slub_min_objects;
	while (min_objects > 1) {
		fraction = 8;
		while (fraction >= 4) {
			order = slab_order(size, min_objects,
						slub_max_order, fraction);
			if (order <= slub_max_order)
				return order;
			fraction /= 2;
		}
		min_objects /= 2;
	}

	/*
	 * We were unable to place multiple objects in a slab. Now
	 * lets see if we can place a single object there.
	 */
	order = slab_order(size, 1, slub_max_order, 1);
	if (order <= slub_max_order)
		return order;

	/*
	 * Doh this slab cannot be placed using slub_max_order.
	 */
	order = slab_order(size, 1, MAX_ORDER, 1);
	if (order <= MAX_ORDER)
		return order;
	return -ENOSYS;
}

/*
 * Figure out what the alignment of the objects will be.
 */
static unsigned long calculate_alignment(unsigned long flags,
		unsigned long align, unsigned long size)
{
	/*
	 * If the user wants hardware cache aligned objects then
	 * follow that suggestion if the object is sufficiently
	 * large.
	 *
	 * The hardware cache alignment cannot override the
	 * specified alignment though. If that is greater
	 * then use it.
	 */
	if ((flags & SLAB_HWCACHE_ALIGN) &&
			size > cache_line_size() / 2)
		return max_t(unsigned long, align, cache_line_size());

	if (align < ARCH_SLAB_MINALIGN)
		return ARCH_SLAB_MINALIGN;

	return ALIGN(align, sizeof(void *));
}

static void init_kmem_cache_cpu(struct kmem_cache *s,
			struct kmem_cache_cpu *c)
{
	c->page = NULL;
	c->freelist = NULL;
	c->offset = s->offset / sizeof(void *);
	c->objsize = s->objsize;
}

static void init_kmem_cache_node(struct kmem_cache_node *n)
{
	n->nr_partial = 0;
	atomic_long_set(&n->nr_slabs, 0);
	spin_lock_init(&n->list_lock);
	INIT_LIST_HEAD(&n->partial);
}

/*
 * Per cpu array for per cpu structures.
 *
 * The per cpu array places all kmem_cache_cpu structures from one processor
 * close together meaning that it becomes possible that multiple per cpu
 * structures are contained in one cacheline. This may be particularly
 * beneficial for the kmalloc caches.
 *
 * A desktop system typically has around 60-80 slabs. With 100 here we are
 * likely able to get per cpu structures for all caches from the array defined
 * here. We must be able to cover all kmalloc caches during bootstrap.
 *
 * If the per cpu array is exhausted then fall back to kmalloc
 * of individual cachelines. No sharing is possible then.
 */
#define NR_KMEM_CACHE_CPU 100

static struct kmem_cache_cpu kmem_cache_cpu[CONFIG_NR_CPUS][NR_KMEM_CACHE_CPU];
static struct kmem_cache_cpu *kmem_cache_cpu_free[CONFIG_NR_CPUS];
static cpumask_t kmem_cach_cpu_free_init_once = CPU_MASK_NONE;

static struct kmem_cache_cpu *alloc_kmem_cache_cpu(struct kmem_cache *s,
							int cpu, gfp_t flags)
{
	struct kmem_cache_cpu *c = kmem_cache_cpu_free[cpu];

	if (c)
		kmem_cache_cpu_free[cpu] = (void *)c->freelist;
	else {
		/* Table overflow: So allocate ourselves */
		c = kmalloc(
			ALIGN(sizeof(struct kmem_cache_cpu), cache_line_size()), flags);
		if (!c)
			return NULL;
	}

	init_kmem_cache_cpu(s, c);
	return c;
}

static void free_kmem_cache_cpu(struct kmem_cache_cpu *c, int cpu)
{
	if (c < kmem_cache_cpu[cpu] ||
			c > kmem_cache_cpu[cpu] + NR_KMEM_CACHE_CPU) {
		kfree(c);
		return;
	}
	c->freelist = (void *)kmem_cache_cpu_free[cpu];
	kmem_cache_cpu_free[cpu] = c;
}

static void free_kmem_cache_cpus(struct kmem_cache *s)
{
	int cpu;

	for_each_online_cpu(cpu) {
		struct kmem_cache_cpu *c = get_cpu_slab(s, cpu);

		if (c) {
			s->cpu_slab[cpu] = NULL;
			free_kmem_cache_cpu(c, cpu);
		}
	}
}

static int alloc_kmem_cache_cpus(struct kmem_cache *s, gfp_t flags)
{
	int cpu;

	for_each_online_cpu(cpu) {
		struct kmem_cache_cpu *c = get_cpu_slab(s, cpu);

		if (c)
			continue;

		c = alloc_kmem_cache_cpu(s, cpu, flags);
		if (!c) {
			free_kmem_cache_cpus(s);
			return 0;
		}
		s->cpu_slab[cpu] = c;
	}
	return 1;
}

/*
 * Initialize the per cpu array.
 */
static void init_alloc_cpu_cpu(int cpu)
{
	int i;

	if (cpumask_test_cpu(cpu, &kmem_cach_cpu_free_init_once))
		return;

	for (i = NR_KMEM_CACHE_CPU - 1; i >= 0; i--)
		free_kmem_cache_cpu(&kmem_cache_cpu[cpu][i], cpu);

	cpumask_set_cpu(cpu, &kmem_cach_cpu_free_init_once);
}

static void __init init_alloc_cpu(void)
{
	int cpu;

	for_each_online_cpu(cpu)
		init_alloc_cpu_cpu(cpu);
}

/*
 * calculate_sizes() determines the order and the distribution of data within
 * a slab object.
 */
static int calculate_sizes(struct kmem_cache *s)
{
	unsigned long flags = s->flags;
	unsigned long size = s->objsize;
	unsigned long align = s->align;

	/*
	 * Determine if we can poison the object itself. If the user of
	 * the slab may touch the object after free or before allocation
	 * then we should never poison the object itself.
	 */
	if ((flags & SLAB_POISON) && !s->ctor)
		s->flags |= __OBJECT_POISON;
	else
		s->flags &= ~__OBJECT_POISON;

	/*
	 * Round up object size to the next word boundary. We can only
	 * place the free pointer at word boundaries and this determines
	 * the possible location of the free pointer.
	 */
	size = ALIGN(size, sizeof(void *));

	/*
	 * With that we have determined the number of bytes in actual use
	 * by the object. This is the potential offset to the free pointer.
	 */
	s->inuse = size;

	if (((flags & (SLAB_POISON)) || s->ctor)) {
		/*
		 * Relocate free pointer after the object if it is not
		 * permitted to overwrite the first word of the object on
		 * kmem_cache_free.
		 *
		 * This is the case if we do RCU, have a constructor or
		 * destructor or are poisoning the objects.
		 */
		s->offset = size;
		size += sizeof(void *);
	}

	/*
	 * Determine the alignment based on various parameters that the
	 * user specified and the dynamic determination of cache line size
	 * on bootup.
	 */
	align = calculate_alignment(flags, align, s->objsize);

	/*
	 * SLUB stores one object immediately after another beginning from
	 * offset 0. In order to align the objects we have to simply size
	 * each object to conform to the alignment.
	 */
	size = ALIGN(size, align);
	s->size = size;

	s->order = calculate_order(size);
	if (s->order < 0)
		return 0;

	/*
	 * Determine the number of objects per slab
	 */
	s->objects = (PAGE_SIZE << s->order) / size;

	return !!s->objects;

}

static int kmem_cache_open(struct kmem_cache *s, gfp_t gfpflags,
		const char *name, size_t size,
		size_t align, unsigned long flags,
		void (*ctor)(struct kmem_cache *, void *))
{
	memset(s, 0, kmem_size);
	s->name = name;
	s->ctor = ctor;
	s->objsize = size;
	s->align = align;
	s->flags = flags;

	if (!calculate_sizes(s))
		goto error;

	s->refcount = 1;

	init_kmem_cache_node(&s->local_node);

	if (alloc_kmem_cache_cpus(s, gfpflags & ~GFP_DMA))
		return 1;

error:
	if (flags & SLAB_PANIC)
		hang("Cannot create slab %s size=%lu realsize=%u "
			"order=%u offset=%u flags=%lx\n",
			s->name, (unsigned long)size, s->size, s->order,
			s->offset, flags);
	return 0;
}

/*
 * Check if a given pointer is valid
 */
int kmem_ptr_validate(struct kmem_cache *s, const void *object)
{
	struct page * page;

	page = get_object_page(object);

	if (!page || s != page->slab)
		/* No slab or wrong slab */
		return 0;

	if (!check_valid_pointer(s, page, object))
		return 0;

	/*
	 * We could also check if the object is on the slabs freelist.
	 * But this would be too expensive and it seems that the main
	 * purpose of kmem_ptr_valid is to check if the object belongs
	 * to a certain slab.
	 */
	return 1;
}

/*
 * Determine the size of a slab object
 */
unsigned int kmem_cache_size(struct kmem_cache *s)
{
	return s->objsize;
}

const char *kmem_cache_name(struct kmem_cache *s)
{
	return s->name;
}

/*
 * Attempt to free all slabs on a node. Return the number of slabs we
 * were unable to free.
 */
static int free_list(struct kmem_cache *s, struct kmem_cache_node *n,
			struct list_head *list)
{
	int slabs_inuse = 0;
	u64 flags;
	struct page *page, *h;

	spin_lock_irqsave(&n->list_lock, flags);
	list_for_each_entry_safe(page, h, list, lru)
		if (!page->inuse) {
			list_del(&page->lru);
			discard_slab(s, page);
		} else
			slabs_inuse++;
	spin_unlock_irqrestore(&n->list_lock, flags);
	return slabs_inuse;
}

/*
 * Release all resources used by a slab cache.
 */
static inline int kmem_cache_close(struct kmem_cache *s)
{
	struct kmem_cache_node *n = get_node(s);

	flush_all(s);

	/* Attempt to free all objects */
	free_kmem_cache_cpus(s);

	n->nr_partial -= free_list(s, n, &n->partial);
	if (atomic_long_read(&n->nr_slabs))
		return 1;

	return 0;
}

/*
 * Close a cache and release the kmem_cache structure
 * (must be used for caches created using kmem_cache_create)
 */
void kmem_cache_destroy(struct kmem_cache *s)
{
	mutex_lock(&slub_lock);
	s->refcount--;
	if (!s->refcount) {
		list_del(&s->list);
		mutex_unlock(&slub_lock);
		if (kmem_cache_close(s))
			WARN_ON(1);
		kfree(s);
	} else
		mutex_unlock(&slub_lock);
}

/********************************************************************
 *		Kmalloc subsystem
 *******************************************************************/

struct kmem_cache kmalloc_caches[PAGE_SHIFT] __cacheline_aligned;

static struct kmem_cache *create_kmalloc_cache(struct kmem_cache *s,
		const char *name, int size, gfp_t gfp_flags)
{
	unsigned int flags = 0;

	if (gfp_flags & GFP_DMA)
		flags = SLAB_CACHE_DMA;

	mutex_lock(&slub_lock);
	if (!kmem_cache_open(s, gfp_flags, name, size, ARCH_KMALLOC_MINALIGN,
			flags, NULL))
		goto panic;

	list_add(&s->list, &slab_caches);
	mutex_unlock(&slub_lock);

	return s;

panic:
	hang("Creation of kmalloc slab %s size=%d failed.\n", name, size);
}

/*
 * Conversion table for small slabs sizes / 8 to the index in the
 * kmalloc array. This is necessary for slabs < 192 since we have non power
 * of two cache sizes there. The size of larger slabs can be determined using
 * fls.
 */
static s8 size_index[24] = {
	3,	/* 8 */
	4,	/* 16 */
	5,	/* 24 */
	5,	/* 32 */
	6,	/* 40 */
	6,	/* 48 */
	6,	/* 56 */
	6,	/* 64 */
	1,	/* 72 */
	1,	/* 80 */
	1,	/* 88 */
	1,	/* 96 */
	7,	/* 104 */
	7,	/* 112 */
	7,	/* 120 */
	7,	/* 128 */
	2,	/* 136 */
	2,	/* 144 */
	2,	/* 152 */
	2,	/* 160 */
	2,	/* 168 */
	2,	/* 176 */
	2,	/* 184 */
	2	/* 192 */
};

static struct kmem_cache *get_slab(size_t size, gfp_t flags)
{
	int index;

	if (size <= 192) {
		if (!size)
			return ZERO_SIZE_PTR;

		index = size_index[(size - 1) / 8];
	} else
		index = fls(size - 1);

	return &kmalloc_caches[index];
}

void *__kmalloc(size_t size, gfp_t flags)
{
	struct kmem_cache *s;

	if (unlikely(size > PAGE_SIZE / 2))
		return (void *)__get_free_pages(flags,
							get_order(size));

	s = get_slab(size, flags);

	if (unlikely(ZERO_OR_NULL_PTR(s)))
		return s;

	return slab_alloc(s, flags, __builtin_return_address(0));
}

size_t ksize(const void *object)
{
	struct page *page;
	struct kmem_cache *s;

	BUG_ON(!object);
	if (unlikely(object == ZERO_SIZE_PTR))
		return 0;

	page = virt_to_head_page(object);
	BUG_ON(!page);

	if (unlikely(!PageSlub(page)))
		return PAGE_SIZE << compound_order(page);

	s = page->slab;
	BUG_ON(!s);

	/*
	 * Debugging requires use of the padding between object
	 * and whatever may come after it.
	 */
	if (s->flags & (SLAB_RED_ZONE | SLAB_POISON))
		return s->objsize;

	/*
	 * If we have the need to store the freelist pointer
	 * back there or track user information then we can
	 * only use the space before that information.
	 */
	if (s->flags & (SLAB_STORE_USER))
		return s->inuse;

	/*
	 * Else we can use all the padding etc for the allocation
	 */
	return s->size;
}

void kfree(const void *x)
{
	struct page *page;

	if (unlikely(ZERO_OR_NULL_PTR(x)))
		return;

	page = virt_to_head_page(x);
	if (unlikely(!PageSlub(page))) {
		put_page(page);
		return;
	}
	slab_free(page->slab, page, (void *)x, __builtin_return_address(0));
}

/********************************************************************
 *			Basic setup of slabs
 *******************************************************************/

void __init kmem_cache_init(void)
{
	int i;
	int caches = 0;

	init_alloc_cpu();

	/* Able to allocate the per node structures */
	slab_state = PARTIAL;

	/* Caches that are not of the two-to-the-power-of size */
	if (KMALLOC_MIN_SIZE <= 64) {
		create_kmalloc_cache(&kmalloc_caches[1],
				"kmalloc-96", 96, GFP_KERNEL);
		caches++;
	}
	if (KMALLOC_MIN_SIZE <= 128) {
		create_kmalloc_cache(&kmalloc_caches[2],
				"kmalloc-192", 192, GFP_KERNEL);
		caches++;
	}

	for (i = KMALLOC_SHIFT_LOW; i < PAGE_SHIFT; i++) {
		create_kmalloc_cache(&kmalloc_caches[i],
			"kmalloc", 1 << i, GFP_KERNEL);
		caches++;
	}


	/*
	 * Patch up the size_index table if we have strange large alignment
	 * requirements for the kmalloc array. This is only the case for
	 * mips it seems. The standard arches will not generate any code here.
	 *
	 * Largest permitted alignment is 256 bytes due to the way we
	 * handle the index determination for the smaller caches.
	 *
	 * Make sure that nothing crazy happens if someone starts tinkering
	 * around with ARCH_KMALLOC_MINALIGN
	 */
	BUILD_BUG_ON(KMALLOC_MIN_SIZE > 256 ||
		(KMALLOC_MIN_SIZE & (KMALLOC_MIN_SIZE - 1)));

	for (i = 8; i < KMALLOC_MIN_SIZE; i += 8)
		size_index[(i - 1) / 8] = KMALLOC_SHIFT_LOW;

	slab_state = UP;

	/* Provide the correct kmalloc names now that the caches are up */
	for (i = KMALLOC_SHIFT_LOW; i < PAGE_SHIFT; i++)
		kmalloc_caches[i].name =
			kasprintf(GFP_KERNEL, "kmalloc-%d", 1 << i);

	kmem_size = offsetof(struct kmem_cache, cpu_slab) +
				nr_cpu_ids * sizeof(struct kmem_cache_cpu *);

	printf("SLUB: Genslabs=%d, HWalign=%d, Order=%d-%d, MinObjects=%d,"
		" CPUs=%ld\n",
		caches, cache_line_size(),
		slub_min_order, slub_max_order, slub_min_objects,
		nr_cpu_ids);
}

/*
 * Find a mergeable slab cache
 */
static int slab_unmergeable(struct kmem_cache *s)
{
	if ((s->flags & SLUB_NEVER_MERGE))
		return 1;

	if (s->ctor)
		return 1;

	/*
	 * We may have set a slab to be unmergeable during bootstrap.
	 */
	if (s->refcount < 0)
		return 1;

	return 0;
}

static struct kmem_cache *find_mergeable(size_t size,
		size_t align, unsigned long flags, const char *name,
		void (*ctor)(struct kmem_cache *, void *))
{
	struct kmem_cache *s;

	if ((flags & SLUB_NEVER_MERGE))
		return NULL;

	if (ctor)
		return NULL;

	size = ALIGN(size, sizeof(void *));
	align = calculate_alignment(flags, align, size);
	size = ALIGN(size, align);

	list_for_each_entry(s, &slab_caches, list) {
		if (slab_unmergeable(s))
			continue;

		if (size > s->size)
			continue;

		if ((flags & SLUB_MERGE_SAME) != (s->flags & SLUB_MERGE_SAME))
				continue;
		/*
		 * Check if alignment is compatible.
		 * Courtesy of Adrian Drzewiecki
		 */
		if ((s->size & ~(align -1)) != s->size)
			continue;

		if (s->size - size >= sizeof(void *))
			continue;

		return s;
	}
	return NULL;
}

struct kmem_cache *kmem_cache_create(const char *name, size_t size,
		size_t align, unsigned long flags,
		void (*ctor)(struct kmem_cache *, void *))
{
	struct kmem_cache *s;

	mutex_lock(&slub_lock);
	s = find_mergeable(size, align, flags, name, ctor);
	if (s) {
		int cpu;

		s->refcount++;
		/*
		 * Adjust the object sizes so that we clear
		 * the complete object on kzalloc.
		 */
		s->objsize = max(s->objsize, (int)size);

		/*
		 * And then we need to update the object size in the
		 * per cpu structures
		 */
		for_each_online_cpu(cpu)
			get_cpu_slab(s, cpu)->objsize = s->objsize;
		s->inuse = max_t(int, s->inuse, ALIGN(size, sizeof(void *)));
		mutex_unlock(&slub_lock);

		return s;
	}
	s = kmalloc(kmem_size, GFP_KERNEL);
	if (s) {
		if (kmem_cache_open(s, GFP_KERNEL, name,
				size, align, flags, ctor)) {
			list_add(&s->list, &slab_caches);
			mutex_unlock(&slub_lock);

			return s;
		}
		kfree(s);
	}
	mutex_unlock(&slub_lock);

	if (flags & SLAB_PANIC)
		hang("Cannot create slabcache %s\n", name);
	else
		s = NULL;
	return s;
}
