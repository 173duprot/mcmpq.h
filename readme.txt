MCMPQ(3)               Library Functions Manual               MCMPQ(3)

NAME
    mcmpq.h - Multi-Consumer Multi-Producer Queue (MCMPQ) library

DESCRIPTION
    mcmpq.h provides a simple, cache-friendly, lock-free
    multi-consumer multi-producer queue implementation in C11.

    This library is designed for high-performance concurrent
    programming where multiple threads may simultaneously
    enqueue and dequeue items without requiring mutexes.

    The queue is based on atomic operations and is optimized
    for minimal contention between threads.

    It allows the following:

        Enqueueing items to the queue,
        Dequeueing items from the queue,
        Checking the size of the queue,
        Checking if the queue is empty.

SYNOPSIS
    #include "mcmpq.h"

    typedef struct {
        _Atomic size_t turn;
        unsigned char storage[ITEM_SIZE];
    } slot_t;

    typedef struct {
        slot_t slots[QUEUE_SIZE];
        _Atomic size_t head;
        _Atomic size_t tail;
    } MPMCQueue;

    void enqueue(MPMCQueue *queue, const void *item);
    bool try_enqueue(MPMCQueue *queue, const void *item);
    void dequeue(MPMCQueue *queue, void *item);
    bool try_dequeue(MPMCQueue *queue, void *item);
    size_t queue_size(MPMCQueue *queue);
    bool queue_empty(MPMCQueue *queue);

FUNCTIONS
    void enqueue(MPMCQueue *queue, const void *item)
        Enqueues an item to the queue. This function
        will busy-wait if the queue is full.

    bool try_enqueue(MPMCQueue *queue, const void *item)
        Attempts to enqueue an item to the queue without
        busy-waiting. Returns true on success or false if
        the queue is full.

    void dequeue(MPMCQueue *queue, void *item)
        Dequeues an item from the queue. This function
        will busy-wait if the queue is empty.

    bool try_dequeue(MPMCQueue *queue, void *item)
        Attempts to dequeue an item from the queue without
        busy-waiting. Returns true on success or false if
        the queue is empty.

    size_t queue_size(MPMCQueue *queue)
        Returns the current number of items in the queue.

    bool queue_empty(MPMCQueue *queue)
        Checks if the queue is empty. Returns true if the
        queue is empty, false otherwise.

CONSTANTS
    QUEUE_SIZE
        Number of slots in the queue (10).

    ITEM_SIZE
        Size of each item in the queue (rest of the cache line).

    CACHE_LINE_SIZE
        Cache line size for alignment (64 bytes).

AUTHOR
    173duprot <https://github.com/173duprot>

COPYRIGHT
    This library is free software: you can redistribute 
    it and/or modify it under the terms of the GNU 
    General Public License as published by the Free 
    Software Foundation, either version 3 of the 
    License, or (at your option) any later version.

    This library is distributed in the hope that it 
    will be useful, but WITHOUT ANY WARRANTY; without 
    even the implied warranty of MERCHANTABILITY or 
    FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
    General Public License for more details.

    You should have received a copy of the GNU General 
    Public License along with this library. If not, 
    see <https://www.gnu.org/licenses/>.

MCMPQ(3)               Library Functions Manual               MCMPQ(3)
