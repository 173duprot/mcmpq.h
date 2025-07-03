#include <stddef.h>
#include <stdatomic.h>
#include <string.h>
#include <stdbool.h>
#include <stdalign.h>

#define SLOTS 10
#define SLOT sizeof(int)

#define CACHE_LINE 64
#define HEAD(q) atomic_load_explicit(&(q)->head, memory_order_acquire)
#define TAIL(q) atomic_load_explicit(&(q)->tail, memory_order_acquire)

typedef struct {
     _Atomic size_t turn;
     unsigned char data[SLOT];
} slot_t;

typedef struct {
    alignas(CACHE_LINE) _Atomic size_t head;
    alignas(CACHE_LINE) _Atomic size_t tail;
    alignas(CACHE_LINE) slot_t slots[SLOTS];
} queue_t;

static inline void enqueue(queue_t *queue, const void *item) {
    size_t head = atomic_fetch_add_explicit(&queue->head, 1, memory_order_acq_rel);
    slot_t *slot = &queue->slots[head % SLOTS];
    while ((head / SLOTS) * 2 != atomic_load_explicit(&slot->turn, memory_order_acquire)) { /* busy-wait */ }
    memcpy(slot->data, item, SLOT);
    atomic_store_explicit(&slot->turn, (head / SLOTS) * 2 + 1, memory_order_release);
}

static inline void dequeue(queue_t *queue, void *item) {
    size_t tail = atomic_fetch_add_explicit(&queue->tail, 1, memory_order_acq_rel);
    slot_t *slot = &queue->slots[tail % SLOTS];
    while ((tail / SLOTS) * 2 + 1 != atomic_load_explicit(&slot->turn, memory_order_acquire)) { /* busy-wait */ }
    memcpy(item, slot->data, SLOT);
    atomic_store_explicit(&slot->turn, (tail / SLOTS) * 2 + 2, memory_order_release);
}
