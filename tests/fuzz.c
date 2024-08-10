#include <stdio.h>
#include <pthread.h>
#include <assert.h>
#include <string.h>
#include "../mcmpq.h"

#define NUM_THREADS 4
#define NUM_ITEMS_PER_THREAD 10000
#define NUM_ITERATIONS 1000

typedef struct {
    int value;
} item_t;

queue_t queue = {	// Global Queue
         .slots = {0},
         .head = 0,
         .tail = 0,
};

void *producer(void *arg) {
    int thread_id = *(int *)arg;
    item_t item_t;
    for (int i = 0; i < NUM_ITEMS_PER_THREAD; ++i) {
        item_t.value = thread_id * NUM_ITEMS_PER_THREAD + i;
        enqueue(&queue, &item_t);
        //printf("Producer %d enqueued: %d\n", thread_id, item_t.value);
    }
    return NULL;
}

void *consumer(void *arg) {
    int thread_id = *(int *)arg;
    item_t item_t;
    for (int i = 0; i < NUM_ITEMS_PER_THREAD; ++i) {
        dequeue(&queue, &item_t);
        //printf("Consumer %d dequeued: %d\n", thread_id, item_t.value);
    }
    return NULL;
}

int fuzz_test(void) {
    pthread_t producers[NUM_THREADS], consumers[NUM_THREADS];
    int thread_ids[NUM_THREADS];

    // Initialize the threads
    for (int i = 0; i < NUM_THREADS; ++i) {
        thread_ids[i] = i;
        pthread_create(&producers[i], NULL, producer, &thread_ids[i]);
        pthread_create(&consumers[i], NULL, consumer, &thread_ids[i]);
    }

    // Wait for all threads to finish
    for (int i = 0; i < NUM_THREADS; ++i) {
        pthread_join(producers[i], NULL);
        pthread_join(consumers[i], NULL);
    }

    // Check that the global queue is empty
    assert(queue_empty(&queue));

    return 0;
}

int main(void) {
    for (int i = 0; i < NUM_ITERATIONS; ++i) {  // Run the fuzz test multiple times to increase the chance of catching race conditions
        fuzz_test();
    }
    printf("\nFuzz test completed successfully. Queue is empty.\n\n\t Itterations: %i\n\n", (NUM_ITEMS_PER_THREAD * NUM_THREADS * NUM_ITERATIONS));
    return 0;
}

