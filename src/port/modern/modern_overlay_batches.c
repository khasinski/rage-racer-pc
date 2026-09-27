#include "modern_overlay_batches.h"

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "../../../external/psyz/external/SDL/src/render/SDL_render_debug_font.h"

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

int ModernOverlayHud(ModernOverlayBatches *batches, const char text[2][64],
                      float logicalWidth, float overscanX) {
    if (!batches || !text || !batches->vertices || !batches->spans ||
        !isfinite(logicalWidth) || logicalWidth <= 0 || !isfinite(overscanX) ||
        batches->vertexCount < 0 || batches->vertexCount > batches->vertexCapacity ||
        batches->spanCount < 0 || batches->spanCount > batches->spanCapacity) return 0;
    const int vertices = batches->vertexCount, spans = batches->spanCount;
    ModernSpan previous = {0};
    if (spans) previous = batches->spans[spans - 1];
    ModernSpan *span = NULL;
    for (int shadow = 1; shadow >= 0; --shadow) {
        for (int line = 0; line < 2; ++line) {
            for (int letter = 0; letter < 36 && text[line][letter]; ++letter) {
                unsigned glyph = (unsigned char)text[line][letter];
                if (glyph < 33 || glyph > 126) continue;
                const Uint8 *pixels = &SDL_RenderDebugTextFontData[(glyph - 33) * 8];
                for (int y = 0; y < 8; ++y) for (int x = 0; x < 8;) {
                    int first = x++;
                    if (!(pixels[y] & (1u << (7 - first)))) continue;
                    while (x < 8 && (pixels[y] & (1u << (7 - x)))) x++;
                    if (!span) span = ModernOverlayBatchesBegin(batches, MODERN_PIPE_2D, NULL);
                    if (!span || batches->vertexCount > batches->vertexCapacity - 6) goto failed;
                    ModernVertex corners[4] = {0};
                    for (int vertex = 0; vertex < 4; ++vertex) {
                        ModernVertex *out = &corners[vertex];
                        float px = (float)(16 + letter * 8 + shadow + ((vertex & 1) ? x : first));
                        float py = (float)(16 + line * 12 + y + shadow + (vertex >> 1));
                        out->x = (px + overscanX) / (logicalWidth * 0.5f) - 1.0f;
                        out->y = -(py / 120.0f - 1.0f);
                        out->w = 1.0f;
                        out->color[0] = out->color[1] = out->color[2] = shadow ? 0 : 240;
                        out->color[3] = 255;
                        out->attr = 0x8000u;
                    }
                    ModernOverlayBatchesEmitQuad(batches, span, corners);
                }
            }
        }
    }
    return 1;
failed:
    batches->vertexCount = vertices;
    batches->spanCount = spans;
    if (spans) batches->spans[spans - 1] = previous;
    return 0;
}
