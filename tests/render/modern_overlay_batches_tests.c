#include "modern_overlay_batches.h"

#include <assert.h>
#include <string.h>

static ModernVertex Vertex(float x, float y) {
    ModernVertex vertex;
    memset(&vertex, 0, sizeof(vertex));
    vertex.x = x;
    vertex.y = y;
    return vertex;
}

int main(void) {
    ModernOverlayBatches batches = {0};
    Modern2DState state;
    ModernSpan *first;
    ModernSpan *second;
    ModernVertex quad[4] = {
        Vertex(0.0f, 0.0f), Vertex(1.0f, 0.0f),
        Vertex(0.0f, 1.0f), Vertex(1.0f, 1.0f),
    };
    ModernVertex triangle[3] = {
        Vertex(2.0f, 0.0f), Vertex(2.0f, 1.0f), Vertex(3.0f, 1.0f),
    };

    assert(!ModernOverlayBatchesAllocate(&batches, 0, 2));
    assert(ModernOverlayBatchesAllocate(&batches, 10, 2));
    ModernOverlayBatchesReset(&batches, 0, 3);
    ModernOverlayStateInit(&state, 240);

    first = ModernOverlayBatchesBegin(&batches, 1, &state);
    assert(first != NULL && first->start == 0 && first->count == 0);
    assert(ModernOverlayBatchesBegin(&batches, 1, &state) == first);
    ModernOverlayBatchesEmitQuad(&batches, first, quad);
    ModernOverlayBatchesEmitTriangle(&batches, first, triangle);
    assert(batches.vertexCount == 9 && batches.spanCount == 1);
    assert(first->count == 9 && batches.vertices[0].x == 0.0f &&
           batches.vertices[8].x == 3.0f);

    assert(!ModernOverlayBatchesPush(&batches, first, triangle, 3));
    assert(batches.vertexCount == 9 && first->count == 9);

    batches.currentLayer = 4;
    second = ModernOverlayBatchesBegin(&batches, 1, &state);
    assert(second != NULL && second != first && second->start == 9 &&
           second->pass == 0 && second->layer == 4 && !second->hasScissor);
    assert(ModernOverlayBatchesPush(&batches, second, triangle, 1));
    assert(second->count == 1 && batches.vertexCount == 10);

    ModernOverlayBatchesRelease(&batches);
    assert(batches.vertices == NULL && batches.spans == NULL &&
           batches.vertexCapacity == 0 && batches.spanCapacity == 0);
    return 0;
}
