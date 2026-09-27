#include "modern_overlay_batches.h"

#include <assert.h>
#include <math.h>
#include <string.h>

static void Hud(void) {
    ModernOverlayBatches batches = {0};
    char text[2][64] = {"!", ""};
    const unsigned char expected[8] = {0x18, 0x3c, 0x3c, 0x18, 0x18, 0, 0x18, 0};
    unsigned char pixels[8] = {0};
    assert(ModernOverlayBatchesAllocate(&batches, 512, 2));
    ModernOverlayBatchesReset(&batches, 0, 3);
    assert(ModernOverlayHud(&batches, text, 320, 0));
    assert(batches.vertexCount == 72 && batches.spanCount == 1);
    assert(batches.spans[0].pipeline == MODERN_PIPE_2D);
    for (int i = 0; i < batches.vertexCount; i += 6) {
        const ModernVertex *v = &batches.vertices[i];
        assert(v[0].attr == 0x8000 && v[0].w == 1 && v[0].color[3] == 255);
        if (i < 36) {
            assert(v[0].color[0] == 0);
            const ModernVertex *foreground = &batches.vertices[i + 36];
            assert(fabsf(v[0].x - foreground[0].x - 1.0f / 160) < 0.00001f);
            assert(fabsf(v[0].y - foreground[0].y + 1.0f / 120) < 0.00001f);
            continue;
        }
        assert(v[0].color[0] == 240 && v[0].color[1] == 240 && v[0].color[2] == 240);
        int x = (int)lroundf((v[0].x + 1) * 160) - 16;
        int end = (int)lroundf((v[1].x + 1) * 160) - 16;
        int y = (int)lroundf((1 - v[0].y) * 120) - 16;
        assert(x >= 0 && end <= 8 && end > x && y >= 0 && y < 8);
        for (; x < end; ++x) pixels[y] |= 1u << (7 - x);
    }
    assert(memcmp(pixels, expected, sizeof(pixels)) == 0);

    ModernOverlayBatchesReset(&batches, 0, 3);
    memset(text, ' ', sizeof(text));
    text[0][35] = text[0][36] = '!';
    assert(ModernOverlayHud(&batches, text, 480, 80));
    assert(batches.vertexCount == 72);
    assert(lroundf((batches.vertices[36].x + 1) * 240 - 80) == 299);

    ModernOverlayBatchesReset(&batches, 0, 3);
    memset(text, 0, sizeof(text));
    text[1][0] = '!';
    assert(ModernOverlayHud(&batches, text, 320, 0));
    assert(batches.vertexCount == 72);
    assert(lroundf((1 - batches.vertices[36].y) * 120) == 28);

    ModernOverlayBatchesReset(&batches, 0, 3);
    ModernSpan *span = ModernOverlayBatchesBegin(&batches, MODERN_PIPE_2D, NULL);
    ModernVertex triangle[3] = {0};
    assert(ModernOverlayBatchesPush(&batches, span, triangle, 3));
    ModernSpan saved = *span;
    batches.vertexCapacity = 40; /* Fail after appending part of the HUD. */
    assert(!ModernOverlayHud(&batches, text, 320, 0));
    assert(batches.vertexCount == 3 && batches.spanCount == 1);
    assert(memcmp(span, &saved, sizeof(saved)) == 0);
    assert(memcmp(batches.vertices, triangle, sizeof(triangle)) == 0);
    assert(!ModernOverlayHud(&batches, text, NAN, 0));
    assert(!ModernOverlayHud(&batches, text, 0, 0));
    assert(!ModernOverlayHud(&batches, NULL, 320, 0));
    batches.currentLayer = 4;
    batches.spanCapacity = 1;
    assert(!ModernOverlayHud(&batches, text, 320, 0));
    assert(batches.vertexCount == 3 && batches.spanCount == 1);
    assert(memcmp(span, &saved, sizeof(saved)) == 0);
    ModernOverlayBatchesReset(&batches, 0, 3);
    memset(text, 0, sizeof(text));
    assert(ModernOverlayHud(&batches, text, 320, 0));
    assert(!batches.vertexCount && !batches.spanCount);
    ModernOverlayBatchesRelease(&batches);
}

static ModernVertex Vertex(float x, float y) {
    ModernVertex vertex;
    memset(&vertex, 0, sizeof(vertex));
    vertex.x = x;
    vertex.y = y;
    return vertex;
}

int main(void) {
    Hud();
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
