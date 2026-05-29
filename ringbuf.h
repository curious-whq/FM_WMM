#ifndef RINGBUF_BUGGY_H
#define RINGBUF_BUGGY_H

#include <stdatomic.h>

#define RINGBUF_OK 0
#define RINGBUF_EMPTY 1
#define RINGBUF_FULL 2

typedef struct {
    _Atomic(void *) *buf;
    atomic_uint head;
    atomic_uint tail;
    unsigned int size;
} ringbuf_t;

static inline void ringbuf_init(ringbuf_t *q, _Atomic(void *) *b, unsigned int s) {
    q->buf = b;
    q->size = s;
    atomic_init(&q->head, 0);
    atomic_init(&q->tail, 0);
    for (unsigned int i = 0; i < s; i++) {
        atomic_init(&q->buf[i], NULL);
    }
}

static inline int ringbuf_enq(ringbuf_t *q, void *v) {
    unsigned int tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    unsigned int head = atomic_load_explicit(&q->head, memory_order_relaxed);
    if (tail - head == q->size) {
        return RINGBUF_FULL;
    }

    atomic_store_explicit(&q->buf[tail % q->size], v, memory_order_relaxed);
    atomic_store_explicit(&q->tail, tail + 1, memory_order_relaxed);
    return RINGBUF_OK;
}

static inline int ringbuf_deq(ringbuf_t *q, void **v) {
    unsigned int head = atomic_load_explicit(&q->head, memory_order_relaxed);

    unsigned int tail = atomic_load_explicit(&q->tail, memory_order_relaxed);
    if (tail - head == 0) {
        return RINGBUF_EMPTY;
    }

    *v = atomic_load_explicit(&q->buf[head % q->size], memory_order_relaxed);
    atomic_store_explicit(&q->head, head + 1, memory_order_relaxed);
    return RINGBUF_OK;
}

#endif
