#include <stdio.h>
#include <pthread.h>
#include "scspq.h"

#define NUM_MESSAGES 10

typedef struct Message {
    alignas(CACHE_LINE_SIZE) int data;
} Message;

SPSCQueue queue;

void* producer_thread(void* arg) {
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        Message msg = {.data = i};
        while (spsc_queue_push(&queue, &msg, sizeof(Message)) == -1) {
            // Queue is full, wait and retry
            sched_yield();
        }
        printf("Produced message: %d\n", msg.data);
    }
    return NULL;
}

void* consumer_thread(void* arg) {
    for (int i = 0; i < NUM_MESSAGES; ++i) {
        Message msg;
        while (spsc_queue_pop(&queue, &msg, sizeof(Message)) == -1) {
            // Queue is empty, wait and retry
            sched_yield();
        }
        printf("Consumed message: %d\n", msg.data);
    }
    return NULL;
}

int main() {
    spsc_queue_init(&queue);

    pthread_t producer, consumer;
    pthread_create(&producer, NULL, producer_thread, NULL);
    pthread_create(&consumer, NULL, consumer_thread, NULL);

    pthread_join(producer, NULL);
    pthread_join(consumer, NULL);

    return 0;
}
