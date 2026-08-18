/* The cache exists because texture pages sit still while the frame buffer is
 * rewritten constantly. These tests hold it to that: a write to the frame
 * buffer must not cost a decode, and a write to the page it came from must. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/port/modern/material_cache.h"
#include "../src/port/modern/material_decode.h"

#define VRAM_W 1024
#define VRAM_H 512
#define PITCH (VRAM_W * 4)

static int failures;
static uint8_t *vram;
static int reads;

#define EXPECT_EQ(expected, actual)                                           \
    do {                                                                      \
        long e = (long)(expected), a = (long)(actual);                        \
        if (e != a) {                                                         \
            fprintf(stderr, "%s:%d: expected %ld, got %ld\n", __FILE__,       \
                    __LINE__, e, a);                                          \
            failures++;                                                       \
        }                                                                     \
    } while (0)

static int ReadVram(int x, int y, int w, int h, void *out) {
    uint8_t *dst = out;
    int row;
    if (x < 0 || y < 0 || x + w > VRAM_W || y + h > VRAM_H) return 0;
    reads++;
    for (row = 0; row < h; row++) {
        memcpy(dst + (size_t)row * w * 4,
               vram + (size_t)(y + row) * PITCH + (size_t)x * 4,
               (size_t)w * 4);
    }
    return 1;
}

static void PutPixel(int x, int y, int r, int g, int b, int a) {
    uint8_t *p = vram + (size_t)y * PITCH + (size_t)x * 4;
    p[0] = (uint8_t)r; p[1] = (uint8_t)g; p[2] = (uint8_t)b; p[3] = (uint8_t)a;
}

static void test_hit_after_miss(void) {
    const uint8_t *first, *second;
    RageMaterialCacheStats stats;

    RageMaterialCacheInit(ReadVram);
    PutPixel(1, 0, 90, 80, 70, 255); /* palette entry 1 */

    first = RageMaterialCacheLookup(0, 0);
    EXPECT_EQ(1, first != NULL);
    second = RageMaterialCacheLookup(0, 0);
    EXPECT_EQ(1, first == second); /* same storage, not re-decoded */

    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(1, stats.misses);
    EXPECT_EQ(1, stats.hits);
    EXPECT_EQ(1, stats.decodes);
    EXPECT_EQ(1, stats.live);
    RageMaterialCacheShutdown();
}

static void test_frame_buffer_writes_do_not_invalidate(void) {
    RageMaterialCacheStats stats;
    RageMaterialCacheInit(ReadVram);

    /* Page 8 sits at x=512 and CLUT 24 at x=384, both clear of the 320-wide
     * frame buffers, which is where the game keeps them: the measured writes
     * land in the first five page columns and nowhere else. */
    EXPECT_EQ(1, RageMaterialCacheLookup(8, 24) != NULL);
    /* A frame's worth of writes to both display buffers. */
    RageMaterialCacheInvalidate(0, 0, 320, 240);
    RageMaterialCacheInvalidate(0, 256, 320, 240);

    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(0, stats.invalidations);
    EXPECT_EQ(1, stats.live);

    RageMaterialCacheLookup(8, 24);
    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(1, stats.hits);
    EXPECT_EQ(1, stats.decodes); /* still the one from the first lookup */
    RageMaterialCacheShutdown();
}

static void test_write_to_the_page_invalidates(void) {
    RageMaterialCacheStats stats;
    RageMaterialCacheInit(ReadVram);

    EXPECT_EQ(1, RageMaterialCacheLookup(8, 24) != NULL);
    RageMaterialCacheInvalidate(512, 0, 8, 8); /* inside page 8 */

    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(1, stats.invalidations);
    EXPECT_EQ(0, stats.live);

    EXPECT_EQ(1, RageMaterialCacheLookup(8, 24) != NULL);
    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(2, stats.decodes); /* decoded again, as it must be */
    RageMaterialCacheShutdown();
}

static void test_write_to_the_palette_invalidates(void) {
    RageMaterialCacheStats stats;
    RageMaterialCacheInit(ReadVram);

    /* CLUT 64 lives at x=0,y=1: a palette can be repainted without the page
     * changing at all, and the decoded colours are then wrong. */
    EXPECT_EQ(1, RageMaterialCacheLookup(8, 64) != NULL);
    RageMaterialCacheInvalidate(0, 1, 16, 1);

    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(1, stats.invalidations);
    RageMaterialCacheShutdown();
}

static void test_16bit_ignores_the_clut_key(void) {
    const uint8_t *plain, *withClut;
    RageMaterialCacheStats stats;
    RageMaterialCacheInit(ReadVram);

    /* Direct colour: the clut field is meaningless, so two lookups that
     * differ only there must share one entry rather than store it twice. */
    plain = RageMaterialCacheLookup(0x100 | 4, 0);
    withClut = RageMaterialCacheLookup(0x100 | 4, 37);
    EXPECT_EQ(1, plain != NULL);
    EXPECT_EQ(1, plain == withClut);
    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(1, stats.decodes);
    EXPECT_EQ(1, stats.live);
    RageMaterialCacheShutdown();
}

static void test_eviction_keeps_the_cache_bounded(void) {
    RageMaterialCacheStats stats;
    int i;
    RageMaterialCacheInit(ReadVram);

    /* More distinct palettes than slots: the cache must stay bounded and
     * start evicting rather than grow. */
    for (i = 0; i < RAGE_MATERIAL_CACHE_SLOTS + 16; i++)
        RageMaterialCacheLookup(0, (unsigned)i);

    stats = RageMaterialCacheGetStats();
    EXPECT_EQ(RAGE_MATERIAL_CACHE_SLOTS, stats.live);
    EXPECT_EQ(16, stats.evictions);
    RageMaterialCacheShutdown();
}

int main(void) {
    vram = calloc((size_t)VRAM_H * PITCH, 1);
    if (vram == NULL) return 1;
    test_hit_after_miss();
    test_frame_buffer_writes_do_not_invalidate();
    test_write_to_the_page_invalidates();
    test_write_to_the_palette_invalidates();
    test_16bit_ignores_the_clut_key();
    test_eviction_keeps_the_cache_bounded();
    free(vram);
    if (failures != 0) {
        fprintf(stderr, "%d material cache assertion(s) failed\n", failures);
        return 1;
    }
    printf("material cache tests passed (%d vram reads)\n", reads);
    return 0;
}
