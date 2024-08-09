#ifndef SPSC_QUEUE_H
#define SPSC_QUEUE_H

#include <stddef.h>
#include <stdint.h>
#define <stdio.h>
#include <string.h>
#include <stdalign.h>

#define MESSAGE_BUFFER_SIZE 64
#define CACHE_LINE_SIZE 64
#define MAX_CAPACITY 1024

typedef struct SPSCQueue {
    size_t write_idx;
    size_t read_idx;
    size_t write_idx_cache;
    size_t read_idx_cache;
    uint8_t data[MAX_CAPACITY];
} SPSCQueue;

static inline void spsc_queue_init(SPSCQueue* queue) {
    if (!queue) return;

    queue->write_idx = 0;
    queue->read_idx = 0;
    queue->write_idx_cache = 0;
    queue->read_idx_cache = 0;
}

static inline int spsc_queue_push(SPSCQueue* queue, const void* data, size_t size) {
    if (!queue || !data || size == 0 || size > MAX_CAPACITY) return -1;

    size_t write_idx = queue->write_idx;
    size_t next_write_idx = (write_idx + 1) % MAX_CAPACITY;

    if (next_write_idx == queue->read_idx_cache) {
        queue->read_idx_cache = queue->read_idx;
        if (next_write_idx == queue->read_idx_cache) return -1;
    }

    char buffer[MESSAGE_BUFFER_SIZE];
    memcpy(buffer, data, size);
    memcpy(&queue->data[write_idx * MESSAGE_BUFFER_SIZE], buffer, MESSAGE_BUFFER_SIZE);
    queue->write_idx = next_write_idx;
    return 0;
}

static inline int spsc_queue_pop(SPSCQueue* queue, void* data, size_t size) {
    if (!queue || !data || size == 0) return -1;

    size_t read_idx = queue->read_idx;

    if (read_idx == queue->write_idx_cache) {
        queue->write_idx_cache = queue->write_idx;
        if (read_idx == queue->write_idx_cache) return -1;
    }

    char buffer[MESSAGE_BUFFER_SIZE];
    memcpy(buffer, &queue->data[read_idx * MESSAGE_BUFFER_SIZE], MESSAGE_BUFFER_SIZE);
    memcpy(data, buffer, size);
    queue->read_idx = (read_idx + 1) % MAX_CAPACITY;
    return 0;
}
#endif // SPSC_QUEUE_H
