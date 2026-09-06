#ifndef RAGE_MODERN_UPLOAD_QUEUE_H
#define RAGE_MODERN_UPLOAD_QUEUE_H

#include <stddef.h>

enum { MODERN_UPLOAD_QUEUE_CAPACITY = 2048 };
typedef struct ModernUploadQueue {
    void *items[MODERN_UPLOAD_QUEUE_CAPACITY];
    size_t count;
} ModernUploadQueue;

static inline int ModernUploadQueueHasRoom(const ModernUploadQueue *queue) {
    return queue != NULL && queue->count < MODERN_UPLOAD_QUEUE_CAPACITY;
}

/* Full queues never release resources: their commands may be unsubmitted.
 * The producer must check capacity before recording commands using an item. */
static inline int ModernUploadQueuePush(ModernUploadQueue *queue, void *item) {
    if (item == NULL || !ModernUploadQueueHasRoom(queue)) return 0;
    queue->items[queue->count++] = item;
    return 1;
}

/* Only after submission or cancellation. The release callback must not
 * mutate this queue. Repeated drains are harmless. */
static inline void ModernUploadQueueDrain(
    ModernUploadQueue *queue, void (*release)(void *, void *), void *context) {
    size_t index;
    if (queue == NULL || release == NULL) return;
    for (index = 0; index < queue->count; ++index) {
        release(context, queue->items[index]);
        queue->items[index] = NULL;
    }
    queue->count = 0;
}

#endif
