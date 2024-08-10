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
    _Atomic unsigned char storage[ITEM_SIZE];  // Storage for the item as atomic
} slot_t;

typedef struct {
    alignas(CACHE_LINE_SIZE) slot_t slots[QUEUE_SIZE];
    alignas(CACHE_LINE_SIZE) _Atomic size_t head;
    alignas(CACHE_LINE_SIZE) _Atomic size_t tail;
} queue_t;

// Example queue init
//
//     queue_t queue = {
//         .slots = {0},
//         .head = 0,
//         .tail = 0,
//     };
//

_Static_assert(alignof(slot_t) == CACHE_LINE_SIZE, "slot_t alignment is incorrect");
_Static_assert(sizeof(slot_t) % CACHE_LINE_SIZE == 0, "slot_t size is not a multiple of cache line size");

static inline void slot_construct(slot_t *slot, const void *item, size_t item_size) {
    for (size_t i = 0; i < item_size; i++) {
        atomic_store_explicit(&slot->storage[i], ((unsigned char *)item)[i], memory_order_release);
    }
}

static inline void slot_destroy(slot_t *slot, size_t item_size) {
    for (size_t i = 0; i < item_size; i++) {
        atomic_store_explicit(&slot->storage[i], 0, memory_order_release);
    }
}

static inline void slot_move(slot_t *slot, void *item, size_t item_size) {
    for (size_t i = 0; i < item_size; i++) {
        ((unsigned char *)item)[i] = atomic_load_explicit(&slot->storage[i], memory_order_acquire);
    }
    slot_destroy(slot, item_size);
}

static inline size_t idx(size_t i) {
    return i % QUEUE_SIZE;
}

static inline size_t turn(size_t i) {
    return i / QUEUE_SIZE;
}

static inline void enqueue(queue_t *queue, const void *item) {
    size_t head = atomic_fetch_add_explicit(&queue->head, 1, memory_order_acq_rel);
    slot_t *slot = &queue->slots[idx(head)];
    while (turn(head) * 2 != atomic_load_explicit(&slot->turn, memory_order_acquire)) {
        // busy-wait
    }
    slot_construct(slot, item, ITEM_SIZE);
    atomic_store_explicit(&slot->turn, turn(head) * 2 + 1, memory_order_release);
}

static inline bool try_enqueue(queue_t *queue, const void *item) {
    size_t head = atomic_load_explicit(&queue->head, memory_order_acquire);
    for (;;) {
        slot_t *slot = &queue->slots[idx(head)];
        if (turn(head) * 2 == atomic_load_explicit(&slot->turn, memory_order_acquire)) {
            if (atomic_compare_exchange_strong_explicit(&queue->head, &head, head + 1, memory_order_acq_rel, memory_order_acquire)) {
                slot_construct(slot, item, ITEM_SIZE);
                atomic_store_explicit(&slot->turn, turn(head) * 2 + 1, memory_order_release);
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

static inline void dequeue(queue_t *queue, void *item) {
    size_t tail = atomic_fetch_add_explicit(&queue->tail, 1, memory_order_acq_rel);
    slot_t *slot = &queue->slots[idx(tail)];
    while (turn(tail) * 2 + 1 != atomic_load_explicit(&slot->turn, memory_order_acquire)) {
        // busy-wait
    }
    slot_move(slot, item, ITEM_SIZE);
    atomic_store_explicit(&slot->turn, turn(tail) * 2 + 2, memory_order_release);
}

static inline bool try_dequeue(queue_t *queue, void *item) {
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
    for (;;) {
        slot_t *slot = &queue->slots[idx(tail)];
        if (turn(tail) * 2 + 1 == atomic_load_explicit(&slot->turn, memory_order_acquire)) {
            if (atomic_compare_exchange_strong_explicit(&queue->tail, &tail, tail + 1, memory_order_acq_rel, memory_order_acquire)) {
                slot_move(slot, item, ITEM_SIZE);
                atomic_store_explicit(&slot->turn, turn(tail) * 2 + 2, memory_order_release);
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

static inline size_t queue_size(queue_t *queue) {
    size_t head = atomic_load_explicit(&queue->head, memory_order_acquire);
    size_t tail = atomic_load_explicit(&queue->tail, memory_order_acquire);
    return head - tail;
}

static inline bool queue_empty(queue_t *queue) {
    return queue_size(queue) == 0;
}
