#pragma once

#include <stddef.h>
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>

#define CACHE_LINE_SIZE 64
#define QUEUE_SIZE 10
#define ITEM_SIZE sizeof(void *)

typedef struct {
    alignas(CACHE_LINE_SIZE) _Atomic size_t turn;
    unsigned char storage[ITEM_SIZE];  // Storage for the item
} slot_t;

typedef struct {
    alignas(CACHE_LINE_SIZE) slot_t slots[QUEUE_SIZE];
    alignas(CACHE_LINE_SIZE) _Atomic size_t head;
    alignas(CACHE_LINE_SIZE) _Atomic size_t tail;
} MPMCQueue;

// Example queue init
//
//     MPMCQueue queue = {
//         .slots = {0},
//         .head = 0,
//         .tail = 0,
//     };
//

_Static_assert(alignof(slot_t) == CACHE_LINE_SIZE, "slot_t alignment is incorrect");
_Static_assert(sizeof(slot_t) % CACHE_LINE_SIZE == 0, "slot_t size is not a multiple of cache line size");

static inline void slot_construct(slot_t *slot, const void *item, size_t item_size) {
    memcpy(slot->storage, item, item_size);
}

static inline void slot_destroy(slot_t *slot, size_t item_size) {
    memset(slot->storage, 0, item_size);
}

static inline void slot_move(slot_t *slot, void *item, size_t item_size) {
    memcpy(item, slot->storage, item_size);
    slot_destroy(slot, item_size);
}

static inline size_t idx(MPMCQueue *queue, size_t i) {
    return i % QUEUE_SIZE;
}

static inline size_t turn(MPMCQueue *queue, size_t i) {
    return i / QUEUE_SIZE;
}

static inline void enqueue(MPMCQueue *queue, const void *item) {
    size_t head = atomic_fetch_add(&queue->head, 1);
    slot_t *slot = &queue->slots[idx(queue, head)];
    while (turn(queue, head) * 2 != atomic_load_explicit(&slot->turn, memory_order_acquire)) {
        // busy-wait
    }
    slot_construct(slot, item, ITEM_SIZE);
    atomic_store_explicit(&slot->turn, turn(queue, head) * 2 + 1, memory_order_release);
}

static inline bool try_enqueue(MPMCQueue *queue, const void *item) {
    size_t head = atomic_load_explicit(&queue->head, memory_order_acquire);
    for (;;) {
        slot_t *slot = &queue->slots[idx(queue, head)];
        if (turn(queue, head) * 2 == atomic_load_explicit(&slot->turn, memory_order_acquire)) {
            if (atomic_compare_exchange_strong(&queue->head, &head, head + 1)) {
                slot_construct(slot, item, ITEM_SIZE);
                atomic_store_explicit(&slot->turn, turn(queue, head) * 2 + 1, memory_order_release);
                return true;
            }
        } else {
            size_t prev_head = head;
            head = atomic_load_explicit(&queue->head, memory_order_acquire);
            if (head == prev_head) {
                return false;
            }
        }
    }
}

static inline void dequeue(MPMCQueue *queue, void *item) {
    size_t tail = atomic_fetch_add(&queue->tail, 1);
    slot_t *slot = &queue->slots[idx(queue, tail)];
    while (turn(queue, tail) * 2 + 1 != atomic_load_explicit(&slot->turn, memory_order_acquire)) {
        // busy-wait
    }
    slot_move(slot, item, ITEM_SIZE);
    atomic_store_explicit(&slot->turn, turn(queue, tail) * 2 + 2, memory_order_release);
}

static inline bool try_dequeue(MPMCQueue *queue, void *item) {
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
    for (;;) {
        slot_t *slot = &queue->slots[idx(queue, tail)];
        if (turn(queue, tail) * 2 + 1 == atomic_load_explicit(&slot->turn, memory_order_acquire)) {
            if (atomic_compare_exchange_strong(&queue->tail, &tail, tail + 1)) {
                slot_move(slot, item, ITEM_SIZE);
                atomic_store_explicit(&slot->turn, turn(queue, tail) * 2 + 2, memory_order_release);
                return true;
            }
        } else {
            size_t prev_tail = tail;
            tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
            if (tail == prev_tail) {
                return false;
            }
        }
    }
}

static inline size_t queue_size(MPMCQueue *queue) {
    return atomic_load_explicit(&queue->head, memory_order_relaxed) - atomic_load_explicit(&queue->tail, memory_order_relaxed);
}

static inline bool queue_empty(MPMCQueue *queue) {
    return queue_size(queue) <= 0;
}

