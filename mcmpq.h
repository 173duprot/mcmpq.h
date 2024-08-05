#include <stdatomic.h>
#include <stddef.h>
#include <string.h>

#define QUEUE_CAPACITY 1024  // Define a fixed capacity
#define CACHE_LINE_SIZE 64

typedef struct {
    _Atomic size_t turn;
    char storage[];
} Slot;

typedef struct {
    size_t capacity;
    size_t item_size;
    char pad1[CACHE_LINE_SIZE - sizeof(size_t) * 2];
    _Atomic size_t head;
    char pad2[CACHE_LINE_SIZE - sizeof(_Atomic size_t)];
    _Atomic size_t tail;
    char pad3[CACHE_LINE_SIZE - sizeof(_Atomic size_t)];
    Slot slots[QUEUE_CAPACITY];
} Queue;

void queue_init(Queue *queue, size_t item_size) {
    queue->capacity = QUEUE_CAPACITY;
    queue->item_size = item_size;
    atomic_store(&queue->head, 0);
    atomic_store(&queue->tail, 0);

    // Initialize turns
    for (size_t i = 0; i < queue->capacity; i++) {
        atomic_store(&queue->slots[i].turn, i * 2);
    }
}

void queue_emplace(Queue *queue, const void *item) {
    size_t head = atomic_fetch_add(&queue->head, 1);
    size_t slot_index = head % queue->capacity;
    size_t turn = 2 * (head / queue->capacity);

    while (atomic_load(&queue->slots[slot_index].turn) != turn) {
        // Spin-wait
    }

    memcpy(queue->slots[slot_index].storage, item, queue->item_size);
    atomic_store(&queue->slots[slot_index].turn, turn + 1);
}

void queue_pop(Queue *queue, void *item) {
    size_t tail = atomic_fetch_add(&queue->tail, 1);
    size_t slot_index = tail % queue->capacity;
    size_t turn = 2 * (tail / queue->capacity) + 1;

    while (atomic_load(&queue->slots[slot_index].turn) != turn) {
        // Spin-wait
    }

    memcpy(item, queue->slots[slot_index].storage, queue->item_size);
    atomic_store(&queue->slots[slot_index].turn, turn + 1);
}
