#include "track_texture_snapshot.h"

#include <stdlib.h>
#include <string.h>

struct RageTrackTextureGeneration {
    uint16_t *vram, *pages[2];
    uint64_t revision;
    size_t references;
    int valid, pagesValid;
};

static size_t PageWords(void) {
    return (size_t)RAGE_TRACK_IMAGE_WIDTH * RAGE_TRACK_IMAGE_HEIGHT;
}

static uint16_t *VramRow(RageTrackTextureGeneration *snapshot, size_t row) {
    return snapshot->vram + (RAGE_TRACK_IMAGE_Y + row) * RAGE_TRACK_VRAM_WIDTH +
           RAGE_TRACK_IMAGE_X;
}

static int BuildPages(RageTrackTextureGeneration *snapshot,
                      const RageTrackTextureSource *source) {
    const size_t rowBytes = RAGE_TRACK_IMAGE_WIDTH * sizeof(uint16_t);
    const size_t vramBytes = (size_t)RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT * sizeof(uint16_t);
    if (source->shadowRows == NULL || source->shadowPages == NULL ||
        source->shadowBytes < PageWords() * sizeof(uint16_t) ||
        source->shadowPageCount < RAGE_TRACK_IMAGE_HEIGHT) return 0;
    for (int page = 0; page < 2; ++page) {
        if (snapshot->pages[page] == NULL)
            snapshot->pages[page] = malloc(vramBytes);
        if (snapshot->pages[page] == NULL) return 0;
        memcpy(snapshot->pages[page], snapshot->vram, vramBytes);
    }
    for (size_t row = 0; row < RAGE_TRACK_IMAGE_HEIGHT; ++row) {
        int shadowPage = source->shadowPages[row] != 0;
        size_t offset = (RAGE_TRACK_IMAGE_Y + row) * RAGE_TRACK_VRAM_WIDTH + RAGE_TRACK_IMAGE_X;
        memcpy(snapshot->pages[shadowPage] + offset,
               (const uint8_t *)source->shadowRows + row * rowBytes, rowBytes);
        memcpy(snapshot->pages[1 - shadowPage] + offset,
               VramRow(snapshot, row), rowBytes);
    }
    snapshot->pagesValid = 1;
    return 1;
}

const uint16_t *TrackTextureGenerationPixels(const RageTrackTextureGeneration *snapshot, int page) {
    if (snapshot == NULL || !snapshot->valid || page < -1 || page > 1) return NULL;
    if (page == -1) return snapshot->vram;
    return snapshot->pagesValid ? snapshot->pages[page] : NULL;
}

const uint16_t *TrackTextureSnapshotSelectPage(RageTrackTextureSnapshot *owner, int page) {
    if (owner == NULL || page < 0) return NULL;
    return TrackTextureGenerationPixels(owner->generation, page);
}

RageTrackTextureGeneration *TrackTextureSnapshotRetain(const RageTrackTextureSnapshot *owner) {
    if (owner == NULL || owner->generation == NULL || !owner->generation->valid ||
        owner->generation->references == SIZE_MAX) return NULL;
    ++owner->generation->references;
    return owner->generation;
}

const uint16_t *TrackTextureSnapshotAcquire(RageTrackTextureSnapshot *owner,
    uint64_t revision, const RageTrackTextureSource *source, int page) {
    const size_t words = (size_t)RAGE_TRACK_VRAM_WIDTH * RAGE_TRACK_VRAM_HEIGHT;
    if (owner == NULL || source == NULL || source->read == NULL ||
        page < -1 || page > 1) return NULL;
    RageTrackTextureGeneration *snapshot = owner->generation;
    if (snapshot != NULL && snapshot->revision != revision && snapshot->references > 1) {
        owner->generation = NULL;
        TrackTextureGenerationRelease(snapshot);
        snapshot = NULL;
    }
    if (snapshot == NULL) {
        snapshot = calloc(1, sizeof(*snapshot));
        if (snapshot == NULL) return NULL;
        snapshot->references = 1;
        owner->generation = snapshot;
    }
    if (!snapshot->valid || snapshot->revision != revision) {
        snapshot->valid = snapshot->pagesValid = 0;
        if (snapshot->vram == NULL)
            snapshot->vram = malloc(words * sizeof(uint16_t));
        if (snapshot->vram == NULL ||
            !source->read(source->context, snapshot->vram, words)) return NULL;
        snapshot->revision = revision;
        snapshot->valid = 1;
    }
    if (page < 0) return snapshot->vram;
    if (!snapshot->pagesValid && !BuildPages(snapshot, source)) return NULL;
    return TrackTextureGenerationPixels(snapshot, page);
}

void TrackTextureGenerationRelease(RageTrackTextureGeneration *snapshot) {
    if (snapshot == NULL) return;
    if (--snapshot->references != 0) return;
    free(snapshot->vram);
    free(snapshot->pages[0]);
    free(snapshot->pages[1]);
    free(snapshot);
}

uint64_t TrackTextureGenerationRevision(const RageTrackTextureGeneration *generation) {
    return generation != NULL && generation->valid ? generation->revision : UINT64_MAX;
}

void TrackTextureSnapshotRelease(RageTrackTextureSnapshot *owner) {
    if (owner == NULL) return;
    TrackTextureGenerationRelease(owner->generation);
    owner->generation = NULL;
}
