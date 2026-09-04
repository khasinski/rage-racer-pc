#include "modern/modern_vram_snapshot.h"

#include <stdio.h>

static int s_failures;
static int s_captureCount;
static int s_failNextCapture;
static char s_textures[4];

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

    first = ModernVramSnapshotForFrame(&cache, 100, Capture, NULL);
    CHECK(first != NULL);
    CHECK(s_captureCount == 1);
    /* 60/120 FPS presentation can render the same logic snapshot several
     * times. Every one must retain the identical frozen VRAM and CLUT. */
    CHECK(ModernVramSnapshotForFrame(&cache, 100, Capture, NULL) == first);
    CHECK(ModernVramSnapshotForFrame(&cache, 100, Capture, NULL) == first);
    CHECK(s_captureCount == 1);

    second = ModernVramSnapshotForFrame(&cache, 101, Capture, NULL);
    CHECK(second != NULL && second != first);
    CHECK(s_captureCount == 2);
    CHECK(ModernVramSnapshotForFrame(&cache, 101, Capture, NULL) == second);
    CHECK(s_captureCount == 2);

    /* A failed GPU snapshot must not poison the frame cache: the next
     * presentation retries instead of reusing the preceding frame. */
    s_failNextCapture = 1;
    CHECK(ModernVramSnapshotForFrame(&cache, 102, Capture, NULL) == NULL);
    CHECK(s_captureCount == 3);
    CHECK(ModernVramSnapshotForFrame(&cache, 102, Capture, NULL) != NULL);
    CHECK(s_captureCount == 4);
    CHECK(ModernVramSnapshotForFrame(NULL, 103, Capture, NULL) == NULL);
    CHECK(ModernVramSnapshotForFrame(&cache, 103, NULL, NULL) == NULL);

    return s_failures != 0;
}
