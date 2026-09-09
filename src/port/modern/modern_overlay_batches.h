#ifndef RAGE_MODERN_OVERLAY_BATCHES_H
#define RAGE_MODERN_OVERLAY_BATCHES_H

#include "modern_overlay_state.h"

#include <stdint.h>

typedef struct ModernVertex {
    float x, y, z, w;
    float u, v;
    uint8_t color[4];
    uint32_t attr;
    uint32_t twin;
    uint32_t clut;
} ModernVertex;

typedef struct ModernSpan {
    uint8_t pipeline;
    uint8_t hasScissor;
    uint8_t pass;
    uint8_t layer;
    SDL_Rect scissor;
    int32_t start;
    int32_t count;
} ModernSpan;

typedef struct ModernOverlayBatches {
    ModernVertex *vertices;
    ModernSpan *spans;
    int vertexCount;
    int spanCount;
    int vertexCapacity;
    int spanCapacity;
    uint8_t currentPass;
    uint8_t currentLayer;
} ModernOverlayBatches;

int ModernOverlayBatchesAllocate(ModernOverlayBatches *batches,
                                  int vertexCapacity, int spanCapacity);
void ModernOverlayBatchesRelease(ModernOverlayBatches *batches);
void ModernOverlayBatchesReset(ModernOverlayBatches *batches,
                               uint8_t pass, uint8_t layer);
ModernSpan *ModernOverlayBatchesBegin(ModernOverlayBatches *batches,
                                      int pipeline,
                                      const Modern2DState *state);
int ModernOverlayBatchesPush(ModernOverlayBatches *batches, ModernSpan *span,
                             const ModernVertex *vertices, int count);
void ModernOverlayBatchesEmitQuad(ModernOverlayBatches *batches,
                                  ModernSpan *span,
                                  const ModernVertex corners[4]);
void ModernOverlayBatchesEmitTriangle(ModernOverlayBatches *batches,
                                      ModernSpan *span,
                                      const ModernVertex corners[3]);

#endif
