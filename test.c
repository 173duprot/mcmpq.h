#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "mcmpq.h"

#define NUM_THREADS 4
#define NUM_ITEMS_PER_THREAD 100

typedef struct {
    int value;
} Item;

MPMCQueue queue;

void *producer(void *arg) {
    int thread_id = *(int *)arg;
    Item item;
    for (int i = 0; i < NUM_ITEMS_PER_THREAD; ++i) {
        item.value = thread_id * NUM_ITEMS_PER_THREAD + i;
        enqueue(&queue, &item);
        printf("Producer %d enqueued: %d\n", thread_id, item.value);
    }
    return NULL;
}

void *consumer(void *arg) {
    int thread_id = *(int *)arg;
    Item item;
    for (int i = 0; i < NUM_ITEMS_PER_THREAD; ++i) {
        dequeue(&queue, &item);
        printf("Consumer %d dequeued: %d\n", thread_id, item.value);
    }
    return NULL;
}

int main() {

    MPMCQueue queue = {
        .slots = {0},
        .head = 0,
        .tail = 0,
    };

    pthread_t producers[NUM_THREADS], consumers[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // Initialize the queue
    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_ids[i] = i;
        pthread_create(&producers[i], NULL, producer, &thread_ids[i]);
        pthread_create(&consumers[i], NULL, consumer, &thread_ids[i]);
    }

    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    assert(queue_empty(&queue));
    printf("Test completed successfully. Queue is empty.\n");

    return 0;
}

