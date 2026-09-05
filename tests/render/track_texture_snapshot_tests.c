#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "port/track_texture_snapshot.h"

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #x); return 1; } } while (0)
typedef struct Fixture {
    uint16_t vram[RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT];
    uint16_t shadow[RAGE_TRACK_IMAGE_WIDTH * RAGE_TRACK_IMAGE_HEIGHT];
    uint8_t shadowPages[RAGE_TRACK_IMAGE_HEIGHT];
    unsigned seed, reads;
    int fail;
} Fixture;

static uint16_t Pixel(unsigned seed, int page, size_t x, size_t y) {
    return (uint16_t)(seed * 73u + page * 9011u + x * 31u + y * 127u);
}
static uint16_t Background(size_t index) { return (uint16_t)(index * 17u); }
static void Fill(Fixture *fixture, unsigned seed) {
    fixture->seed = seed;
    for (size_t i = 0; i < sizeof(fixture->vram) / sizeof(uint16_t); ++i)
        fixture->vram[i] = Background(i);
    for (size_t y = 0; y < RAGE_TRACK_IMAGE_HEIGHT; ++y) {
        int bank = ((y + seed) % 3u) != 0;
        fixture->shadowPages[y] = bank ? 7 : 0; /* All nonzero values mean 1. */
        for (size_t x = 0; x < RAGE_TRACK_IMAGE_WIDTH; ++x) {
            fixture->shadow[y * RAGE_TRACK_IMAGE_WIDTH + x] = Pixel(seed, bank, x, y);
            fixture->vram[(y + RAGE_TRACK_IMAGE_Y) * RAGE_TRACK_VRAM_WIDTH +
                          RAGE_TRACK_IMAGE_X + x] = Pixel(seed, 1 - bank, x, y);
        }
    }
}
static int Read(void *context, uint16_t *words, size_t count) {
    Fixture *fixture = context;
    ++fixture->reads;
    if (fixture->fail || count != sizeof(fixture->vram) / sizeof(uint16_t)) return 0;
    memcpy(words, fixture->vram, sizeof(fixture->vram));
    return 1;
}
static RageTrackTextureSource Source(Fixture *fixture) {
    return (RageTrackTextureSource){Read, fixture,
        fixture->shadow, sizeof(fixture->shadow),
        fixture->shadowPages, sizeof(fixture->shadowPages)};
}
static int Matches(const uint16_t *image, unsigned seed, int page) {
    if (image == NULL) return 0;
    for (size_t y = 0; y < RAGE_TRACK_VRAM_HEIGHT; ++y) {
        for (size_t x = 0; x < RAGE_TRACK_VRAM_WIDTH; ++x) {
            size_t index = y * RAGE_TRACK_VRAM_WIDTH + x;
            uint16_t expected = Background(index);
            if (y >= RAGE_TRACK_IMAGE_Y && x >= RAGE_TRACK_IMAGE_X)
                expected = Pixel(seed, page, x - RAGE_TRACK_IMAGE_X,
                                 y - RAGE_TRACK_IMAGE_Y);
            if (image[index] != expected) return 0;
        }
    }
    return 1;
}
int main(void) {
    Fixture *fixture = calloc(1, sizeof(*fixture));
    Fixture *other = calloc(1, sizeof(*other));
    CHECK(fixture && other);
    Fill(fixture, 1);
    Fill(other, 999);
    RageTrackTextureSource source = Source(fixture), sourceOther = Source(other);
    RageTrackTextureSnapshot a = {0}, b = {0};
    CHECK(TrackTextureSnapshotSelectPage(&a, 0) == NULL);
    const uint16_t *image = TrackTextureSnapshotAcquire(&a, 0, &source, -1);
    CHECK(image && memcmp(image, fixture->vram, sizeof(fixture->vram)) == 0);
    CHECK(fixture->reads == 1 && TrackTextureSnapshotSelectPage(&a, 0) == NULL);
    for (unsigned race = 1; race <= 100; ++race) {
        Fill(fixture, race);
        image = TrackTextureSnapshotAcquire(&a, race, &source, 0);
        CHECK(Matches(image, race, 0));
        const uint16_t *allocation = TrackTextureSnapshotAcquire(&a, race, &source, -1);
        CHECK(Matches(TrackTextureSnapshotSelectPage(&a, 1), race, 1));
        CHECK(Matches(image, race, 0)); /* Bank 1 must not overwrite bank 0. */
        const uint16_t *bank1 = TrackTextureSnapshotSelectPage(&a, 1);
        CHECK(bank1 != image);
        CHECK(Matches(TrackTextureSnapshotAcquire(&a, race, &source, 0), race, 0));
        CHECK(Matches(bank1, race, 1));
        CHECK(TrackTextureSnapshotAcquire(&a, race, &source, -1) == allocation);
        CHECK(memcmp(allocation, fixture->vram, sizeof(fixture->vram)) == 0);
        CHECK(fixture->reads == race + 1);
        /* Loading another owner with the same revision must not affect a. */
        CHECK(Matches(TrackTextureSnapshotAcquire(&b, race, &sourceOther, 1), 999, 1));
        CHECK(Matches(image, race, 0));
    }
    fixture->fail = 1;
    CHECK(TrackTextureSnapshotAcquire(&a, 101, &source, 1) == NULL);
    CHECK(TrackTextureSnapshotRetain(&a) == NULL);
    CHECK(TrackTextureSnapshotSelectPage(&a, 0) == NULL);
    fixture->fail = 0;
    Fill(fixture, 101);
    CHECK(Matches(TrackTextureSnapshotAcquire(&a, 101, &source, 1), 101, 1));
    source.shadowBytes--;
    CHECK(TrackTextureSnapshotAcquire(&a, 102, &source, 0) == NULL);
    CHECK(TrackTextureSnapshotSelectPage(&a, 0) == NULL);
    source.shadowBytes++;
    source.shadowPageCount--;
    CHECK(TrackTextureSnapshotAcquire(&a, 102, &source, 0) == NULL);
    source.shadowPageCount++;
    CHECK(Matches(TrackTextureSnapshotAcquire(&a, 102, &source, 0), 101, 0));
    CHECK(TrackTextureSnapshotAcquire(&a, 102, &source, 2) == NULL);
    CHECK(TrackTextureSnapshotSelectPage(&a, -1) == NULL);
    CHECK(TrackTextureSnapshotAcquire(NULL, 0, &source, 0) == NULL);
    CHECK(TrackTextureSnapshotAcquire(&a, 0, NULL, 0) == NULL);
    unsigned reads = fixture->reads;
    TrackTextureSnapshotRelease(&a);
    TrackTextureSnapshotRelease(&a);
    CHECK(a.generation == NULL);
    CHECK(Matches(TrackTextureSnapshotAcquire(&a, 102, &source, 1), 101, 1));
    CHECK(fixture->reads == reads + 1);
    RageTrackTextureGeneration *retained = TrackTextureSnapshotRetain(&a);
    CHECK(retained != NULL);
    RageTrackTextureGeneration *secondReference = TrackTextureSnapshotRetain(&a);
    CHECK(secondReference == retained);
    const uint16_t *oldBank = TrackTextureGenerationPixels(retained, 1);
    CHECK(Matches(oldBank, 101, 1));
    Fill(fixture, 103);
    fixture->fail = 1;
    CHECK(TrackTextureSnapshotAcquire(&a, 103, &source, 0) == NULL);
    CHECK(TrackTextureSnapshotRetain(&a) == NULL);
    CHECK(Matches(oldBank, 101, 1));
    CHECK(Matches(TrackTextureGenerationPixels(secondReference, 0), 101, 0));
    TrackTextureGenerationRelease(secondReference);
    CHECK(Matches(oldBank, 101, 1));
    fixture->fail = 0;
    CHECK(Matches(TrackTextureSnapshotAcquire(&a, 103, &source, 0), 103, 0));
    CHECK(TrackTextureGenerationPixels(retained, 2) == NULL);
    CHECK(TrackTextureGenerationPixels(NULL, 0) == NULL);
    CHECK(Matches(oldBank, 101, 1));
    TrackTextureSnapshotRelease(&a);
    CHECK(Matches(TrackTextureGenerationPixels(retained, 1), 101, 1));
    TrackTextureGenerationRelease(retained);
    TrackTextureGenerationRelease(NULL);
    TrackTextureSnapshotRelease(&b);
    TrackTextureSnapshotRelease(NULL);
    free(fixture);
    free(other);
    puts("Track image ownership: 100 generations, independent owners, read retry and teardown passed");
    return 0;
}
