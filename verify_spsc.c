#include <assert.h>
#include <pthread.h>
#include <sched.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdatomic.h>
#include <stdio.h>

#ifndef RINGBUF_HEADER
#define RINGBUF_HEADER "ringbuf_buggy.h"
#endif
#include RINGBUF_HEADER

#define RBUF_SIZE 2
#define VALUES 3
#define TOTAL VALUES

struct data {
    atomic_int sent;
    atomic_int recv;
};

_Atomic(void *) buf[RBUF_SIZE];
ringbuf_t rb;
struct data data_items[TOTAL];

static void wait_until_enq_ok(ringbuf_t *q, void *p) {
    while (ringbuf_enq(q, p) != RINGBUF_OK) {
        sched_yield();
    }
}

static void wait_until_deq_ok(ringbuf_t *q, void **p) {
    while (ringbuf_deq(q, p) != RINGBUF_OK) {
        sched_yield();
    }
}

void *producer(void *arg) {
    (void)arg;
    for (int i = 0; i < VALUES; i++) {
        struct data *d = &data_items[i];
        atomic_store_explicit(&d->sent, 1, memory_order_relaxed);
        wait_until_enq_ok(&rb, d);
    }
    return NULL;
}

void *consumer(void *arg) {
    (void)arg;
    for (int i = 0; i < TOTAL; i++) {
        struct data *d = NULL;
        wait_until_deq_ok(&rb, (void **)&d);

        assert(d != NULL);
        assert(atomic_load_explicit(&d->sent, memory_order_relaxed) == 1);
        atomic_store_explicit(&d->recv, 1, memory_order_relaxed);
    }
    return NULL;
}

int main(void) {
    ringbuf_init(&rb, buf, RBUF_SIZE);
    for (int i = 0; i < TOTAL; i++) {
        atomic_init(&data_items[i].sent, 0);
        atomic_init(&data_items[i].recv, 0);
    }

    pthread_t tp, tc;
    pthread_create(&tp, NULL, producer, NULL);
    pthread_create(&tc, NULL, consumer, NULL);
    pthread_join(tp, NULL);
    pthread_join(tc, NULL);

    for (int i = 0; i < TOTAL; i++) {
        assert(atomic_load_explicit(&data_items[i].sent, memory_order_relaxed) == 1);
        assert(atomic_load_explicit(&data_items[i].recv, memory_order_relaxed) == 1);
    }
    puts("finished");
    return 0;
}
