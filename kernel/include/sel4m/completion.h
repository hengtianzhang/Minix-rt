#ifndef __SEL4M_COMPLETION_H_
#define __SEL4M_COMPLETION_H_

struct completion {
	unsigned int done;
//	wait_queue_head_t wait;
};

static inline void init_completion(struct completion *x)
{
	x->done = 0;
//	init_waitqueue_head(&x->wait);
}

static inline void wait_for_completion(struct completion *x)
{

}

#endif /* !__SEL4M_COMPLETION_H_ */
