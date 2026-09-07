#include "modern/modern_vram_snapshot.h"

#include <stdio.h>

static int s_failures;
static int s_captureCount;
static int s_failNextCapture;
static char s_textures[16];

#define CHECK(condition) do {                                              \
    if (!(condition)) {                                                    \
        fprintf(stderr, "%s:%d: check failed: %s\n", __FILE__, __LINE__, \
                #condition);                                               \
        s_failures++;                                                      \
    }                                                                      \
} while (0)

static SDL_GPUTexture *Capture(void *context) {
    (void)context;
    s_captureCount++;
    if (s_failNextCapture) {
        s_failNextCapture = 0;
        return NULL;
    }
    return (SDL_GPUTexture *)&s_textures[s_captureCount - 1];
}

int main(void) {
    ModernVramSnapshotCache cache = {0};
    SDL_GPUTexture *first;
    SDL_GPUTexture *second;

    first = ModernVramSnapshotForFrame(&cache, 100, 1, 1, Capture, NULL);
    CHECK(first != NULL);
    CHECK(s_captureCount == 1);
    /* 60/120 FPS presentation can render the same logic snapshot several
     * times. Every one must retain the identical frozen VRAM and CLUT. */
    CHECK(ModernVramSnapshotForFrame(&cache, 100, 1, 1, Capture, NULL) == first);
    CHECK(ModernVramSnapshotForFrame(&cache, 100, 1, 1, Capture, NULL) == first);
    CHECK(s_captureCount == 1);

    second = ModernVramSnapshotForFrame(&cache, 101, 1, 1, Capture, NULL);
    CHECK(second != NULL && second != first);
    CHECK(s_captureCount == 2);
    CHECK(ModernVramSnapshotForFrame(&cache, 101, 1, 1, Capture, NULL) == second);
    CHECK(s_captureCount == 2);

    /* A failed GPU snapshot must not poison the frame cache: the next
     * presentation retries instead of reusing the preceding frame. */
    s_failNextCapture = 1;
    CHECK(ModernVramSnapshotForFrame(&cache, 102, 1, 1, Capture, NULL) == NULL);
    CHECK(s_captureCount == 3);
    CHECK(ModernVramSnapshotForFrame(&cache, 102, 1, 1, Capture, NULL) != NULL);
    CHECK(s_captureCount == 4);
    second = cache.texture;
    ModernVramSnapshotReset(&cache);
    CHECK(!cache.valid && cache.texture == NULL);
    CHECK(ModernVramSnapshotForFrame(&cache, 102, 1, 1, Capture, NULL) != second);
    CHECK(s_captureCount == 5);
    first = cache.texture;
    /* A repeated scene frame must not reuse pixels from a retired track or
     * asset generation. Both identities independently invalidate the cache. */
    second = ModernVramSnapshotForFrame(&cache, 102, 2, 1, Capture, NULL);
    CHECK(second && second != first && s_captureCount == 6);
    CHECK(ModernVramSnapshotForFrame(&cache, 102, 2, 1, Capture, NULL) == second);
    CHECK(s_captureCount == 6);
    first = ModernVramSnapshotForFrame(&cache, 102, 2, 2, Capture, NULL);
    CHECK(first && first != second && s_captureCount == 7);
    s_failNextCapture = 1;
    CHECK(ModernVramSnapshotForFrame(&cache, 102, 3, 2, Capture, NULL) == NULL);
    CHECK(cache.trackRevision == 2 && cache.assetGeneration == 2);
    CHECK(cache.texture == first && s_captureCount == 8);
    CHECK(ModernVramSnapshotForFrame(&cache, 102, 3, 2, Capture, NULL) != NULL);
    CHECK(cache.trackRevision == 3 && cache.assetGeneration == 2 && s_captureCount == 9);
    CHECK(ModernVramSnapshotForFrame(&cache, 102, UINT64_MAX, UINT64_MAX,
        Capture, NULL) != NULL);
    CHECK(s_captureCount == 10);
    first = cache.texture;
    CHECK(ModernVramSnapshotLookup(&cache, 102, UINT64_MAX, UINT64_MAX) == first);
    CHECK(ModernVramSnapshotLookup(&cache, 103, UINT64_MAX, UINT64_MAX) == NULL);
    CHECK(ModernVramSnapshotLookup(&cache, 102, 0, UINT64_MAX) == NULL);
    CHECK(ModernVramSnapshotLookup(&cache, 102, UINT64_MAX, 0) == NULL);
    CHECK(ModernVramSnapshotLookup(NULL, 102, UINT64_MAX, UINT64_MAX) == NULL);
    CHECK(s_captureCount == 10);
    CHECK(ModernVramSnapshotForFrame(&cache, 102, UINT64_MAX, UINT64_MAX,
        Capture, NULL) == first);
    CHECK(s_captureCount == 10);
    ModernVramSnapshotReset(&cache);
    ModernVramSnapshotReset(&cache);
    ModernVramSnapshotReset(NULL);
    CHECK(ModernVramSnapshotForFrame(NULL, 103, 1, 1, Capture, NULL) == NULL);
    CHECK(ModernVramSnapshotForFrame(&cache, 103, 1, 1, NULL, NULL) == NULL);

    return s_failures != 0;
}
