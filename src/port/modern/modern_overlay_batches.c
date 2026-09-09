#include "modern_overlay_batches.h"

#include <stdlib.h>
#include <string.h>

int ModernOverlayBatchesAllocate(ModernOverlayBatches *batches,
                                  int vertexCapacity, int spanCapacity) {
    ModernVertex *vertices;
    ModernSpan *spans;
    if (!batches || vertexCapacity <= 0 || spanCapacity <= 0) return 0;
    vertices = malloc((size_t)vertexCapacity * sizeof(*vertices));
    spans = malloc((size_t)spanCapacity * sizeof(*spans));
    if (!vertices || !spans) {
        free(vertices);
        free(spans);
        return 0;
    }
    ModernOverlayBatchesRelease(batches);
    batches->vertices = vertices;
    batches->spans = spans;
    batches->vertexCapacity = vertexCapacity;
    batches->spanCapacity = spanCapacity;
    return 1;
}

void ModernOverlayBatchesRelease(ModernOverlayBatches *batches) {
    if (!batches) return;
    free(batches->vertices);
    free(batches->spans);
    memset(batches, 0, sizeof(*batches));
}

void ModernOverlayBatchesReset(ModernOverlayBatches *batches,
                               uint8_t pass, uint8_t layer) {
    if (!batches) return;
    batches->vertexCount = 0;
    batches->spanCount = 0;
    batches->currentPass = pass;
    batches->currentLayer = layer;
}

ModernSpan *ModernOverlayBatchesBegin(ModernOverlayBatches *batches,
                                      int pipeline,
                                      const Modern2DState *state) {
    ModernSpan *span;
    if (!batches) return NULL;
    if (batches->spanCount > 0) {
        span = &batches->spans[batches->spanCount - 1];
        if (span->pipeline == pipeline && span->pass == batches->currentPass &&
            span->layer == batches->currentLayer &&
            ((state == NULL && !span->hasScissor) ||
             (state != NULL && state->hasScissor == span->hasScissor &&
              (!state->hasScissor ||
               (state->scissor.x == span->scissor.x &&
                state->scissor.y == span->scissor.y &&
                state->scissor.w == span->scissor.w &&
                state->scissor.h == span->scissor.h))))) return span;
    }
    if (batches->spanCount >= batches->spanCapacity) return NULL;
    span = &batches->spans[batches->spanCount++];
    span->pipeline = (uint8_t)pipeline;
    span->pass = batches->currentPass;
    span->layer = batches->currentLayer;
    span->hasScissor = state != NULL && state->hasScissor;
    if (span->hasScissor) span->scissor = state->scissor;
    span->start = batches->vertexCount;
    span->count = 0;
    return span;
}

int ModernOverlayBatchesPush(ModernOverlayBatches *batches, ModernSpan *span,
                             const ModernVertex *vertices, int count) {
    if (!batches || !span || !vertices || count <= 0 ||
        batches->vertexCount < 0 ||
        batches->vertexCount > batches->vertexCapacity - count) return 0;
    memcpy(&batches->vertices[batches->vertexCount], vertices,
           (size_t)count * sizeof(*vertices));
    batches->vertexCount += count;
    span->count += count;
    return 1;
}

void ModernOverlayBatchesEmitQuad(ModernOverlayBatches *batches,
                                  ModernSpan *span,
                                  const ModernVertex corners[4]) {
    const ModernVertex triangles[6] = {
        corners[0], corners[1], corners[2], corners[1], corners[3], corners[2],
    };
    (void)ModernOverlayBatchesPush(batches, span, triangles, 6);
}

void ModernOverlayBatchesEmitTriangle(ModernOverlayBatches *batches,
                                      ModernSpan *span,
                                      const ModernVertex corners[3]) {
    (void)ModernOverlayBatchesPush(batches, span, corners, 3);
}
