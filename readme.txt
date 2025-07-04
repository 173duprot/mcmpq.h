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
        Safely accessing the head value.
        Safely accessing the tail value.

SYNOPSIS
    #include "mcmpq.h"

    typedef struct {
        _Atomic size_t turn;
        unsigned char storage[ITEM_SIZE];
    } slot_t;

    typedef struct {
        _Atomic size_t head;
        _Atomic size_t tail;
        slot_t slots[QUEUE_SIZE];
    } queue_t;

    void enqueue(queue_t *queue, const void *item);
    bool try_enqueue(queue_t *queue, const void *item);
    void dequeue(queue_t *queue, void *item);
    bool try_dequeue(queue_t *queue, void *item);

FUNCTIONS
    void enqueue(queue_t *queue, const void *item)
        Enqueues an item to the queue. This function
        will busy-wait if the queue is full.

    bool try_enqueue(queue_t *queue, const void *item)
        Attempts to enqueue an item to the queue without
        busy-waiting. Returns true on success or false if
        the queue is full.

    void dequeue(queue_t *queue, void *item)
        Dequeues an item from the queue. This function
        will busy-wait if the queue is empty.

    bool try_dequeue(queue_t *queue, void *item)
        Attempts to dequeue an item from the queue without
        busy-waiting. Returns true on success or false if
        the queue is empty.

CONSTANTS
    SLOT
        Size of each slot in the queue (int).

    SLOTS
        Number of slots in the queue (10).

MACROS
    HEAD(*queue_t)
        Safely accesses the head value.

    TAIL(*queue_t)
        Safely accesses the tail value.

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
