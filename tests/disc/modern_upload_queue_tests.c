#include "port/modern/modern_upload_queue.h"
#include <assert.h>

typedef struct Item { unsigned releases; } Item;
static void Release(void *context, void *resource) {
    size_t *count = context;
    Item *item = resource;
    assert(item->releases == 0);
    ++item->releases;
    ++*count;
}

int main(void) {
    ModernUploadQueue queue = {0};
    Item items[MODERN_UPLOAD_QUEUE_CAPACITY + 1] = {{0}};
    size_t released = 0, index;
    assert(!ModernUploadQueuePush(NULL, items));
    assert(!ModernUploadQueuePush(&queue, NULL));
    for (index = 0; index < MODERN_UPLOAD_QUEUE_CAPACITY; ++index) {
        assert(ModernUploadQueueHasRoom(&queue));
        assert(ModernUploadQueuePush(&queue, &items[index]));
        assert(queue.count == index + 1);
        assert(released == 0);
    }
    assert(!ModernUploadQueueHasRoom(&queue));
    assert(!ModernUploadQueuePush(&queue, &items[index]));
    assert(queue.count == MODERN_UPLOAD_QUEUE_CAPACITY);
    ModernUploadQueueDrain(&queue, NULL, &released);
    assert(queue.count == MODERN_UPLOAD_QUEUE_CAPACITY);
    ModernUploadQueueDrain(&queue, Release, &released);
    assert(released == MODERN_UPLOAD_QUEUE_CAPACITY);
    assert(queue.count == 0);
    for (index = 0; index < MODERN_UPLOAD_QUEUE_CAPACITY; ++index) {
        assert(items[index].releases == 1);
        assert(queue.items[index] == NULL);
    }
    ModernUploadQueueDrain(&queue, Release, &released);
    assert(released == MODERN_UPLOAD_QUEUE_CAPACITY);
    assert(ModernUploadQueuePush(&queue, &items[MODERN_UPLOAD_QUEUE_CAPACITY]));
    ModernUploadQueueDrain(&queue, Release, &released);
    assert(released == MODERN_UPLOAD_QUEUE_CAPACITY + 1);
    return 0;
}
