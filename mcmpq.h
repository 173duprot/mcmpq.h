/*
Copyright (c) 2020 Erik Rigtorp

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#ifndef MPMCQUEUE_H
#define MPMCQUEUE_H

#include <stdatomic.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <assert.h>

#define CACHE_LINE_SIZE 64

typedef struct Slot {
    _Atomic size_t turn;
    char storage[];
} Slot;

#define SLOT_SIZE(type) (sizeof(Slot) + sizeof(type))

static inline void slot_construct(Slot *slot, void *data, size_t size) {
    memcpy(slot->storage, data, size);
}

static inline void slot_destroy(Slot *slot) {
    // No-op in this context, but here you would call destructor if needed.
}

static inline void *slot_move(Slot *slot) {
    return slot->storage;
}

typedef struct MPMCQueue {
    const size_t capacity;
    Slot *slots;
    _Atomic size_t head;
    _Atomic size_t tail;
} MPMCQueue;

static inline size_t idx(MPMCQueue *queue, size_t i) {
    return i % queue->capacity;
}

static inline size_t turn(MPMCQueue *queue, size_t i) {
    return i / queue->capacity;
}

static inline void MPMCQueue_init(MPMCQueue *queue, size_t capacity, size_t item_size) {
    assert(capacity > 0);
    queue->capacity = capacity;
    queue->head = 0;
    queue->tail = 0;
    queue->slots = (Slot *)aligned_alloc(CACHE_LINE_SIZE, SLOT_SIZE(item_size) * (capacity + 1));
    assert(queue->slots != NULL);
    for (size_t i = 0; i < capacity; ++i) {
        queue->slots[i].turn = ATOMIC_VAR_INIT(0);
    }
}

static inline void MPMCQueue_destroy(MPMCQueue *queue) {
    free(queue->slots);
}

static inline void MPMCQueue_emplace(MPMCQueue *queue, void *data, size_t size) {
    size_t head = atomic_fetch_add(&queue->head, 1);
    Slot *slot = &queue->slots[idx(queue, head)];
    while (turn(queue, head) * 2 != atomic_load(&slot->turn));
    slot_construct(slot, data, size);
    atomic_store(&slot->turn, turn(queue, head) * 2 + 1);
}

static inline bool MPMCQueue_try_emplace(MPMCQueue *queue, void *data, size_t size) {
    size_t head = atomic_load(&queue->head);
    for (;;) {
        Slot *slot = &queue->slots[idx(queue, head)];
        if (turn(queue, head) * 2 == atomic_load(&slot->turn)) {
            if (atomic_compare_exchange_strong(&queue->head, &head, head + 1)) {
                slot_construct(slot, data, size);
                atomic_store(&slot->turn, turn(queue, head) * 2 + 1);
                return true;
            }
        } else {
            size_t prev_head = head;
            head = atomic_load(&queue->head);
            if (head == prev_head) {
                return false;
            }
        }
    }
}

static inline void MPMCQueue_pop(MPMCQueue *queue, void *buffer, size_t size) {
    size_t tail = atomic_fetch_add(&queue->tail, 1);
    Slot *slot = &queue->slots[idx(queue, tail)];
    while (turn(queue, tail) * 2 + 1 != atomic_load(&slot->turn));
    memcpy(buffer, slot_move(slot), size);
    slot_destroy(slot);
    atomic_store(&slot->turn, turn(queue, tail) * 2 + 2);
}

static inline bool MPMCQueue_try_pop(MPMCQueue *queue, void *buffer, size_t size) {
    size_t tail = atomic_load(&queue->tail);
    for (;;) {
        Slot *slot = &queue->slots[idx(queue, tail)];
        if (turn(queue, tail) * 2 + 1 == atomic_load(&slot->turn)) {
            if (atomic_compare_exchange_strong(&queue->tail, &tail, tail + 1)) {
                memcpy(buffer, slot_move(slot), size);
                slot_destroy(slot);
                atomic_store(&slot->turn, turn(queue, tail) * 2 + 2);
                return true;
            }
        } else {
            size_t prev_tail = tail;
            tail = atomic_load(&queue->tail);
            if (tail == prev_tail) {
                return false;
            }
        }
    }
}

static inline ptrdiff_t MPMCQueue_size(MPMCQueue *queue) {
    return (ptrdiff_t)(atomic_load(&queue->head) - atomic_load(&queue->tail));
}

static inline bool MPMCQueue_empty(MPMCQueue *queue) {
    return MPMCQueue_size(queue) <= 0;
}

#endif // MPMCQUEUE_H
