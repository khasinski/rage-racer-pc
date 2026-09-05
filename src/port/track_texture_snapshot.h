#ifndef RAGE_TRACK_TEXTURE_SNAPSHOT_H
#define RAGE_TRACK_TEXTURE_SNAPSHOT_H

#include <stddef.h>
#include <stdint.h>

enum {
    RAGE_TRACK_VRAM_WIDTH = 1024,
    RAGE_TRACK_VRAM_HEIGHT = 512,
    RAGE_TRACK_IMAGE_X = 576,
    RAGE_TRACK_IMAGE_Y = 256,
    RAGE_TRACK_IMAGE_WIDTH = 448,
    RAGE_TRACK_IMAGE_HEIGHT = 256
};

typedef struct RageTrackTextureGeneration RageTrackTextureGeneration;
/* This object owns a generation reference. Zero-initialize before first use;
 * release is idempotent. Do not copy an initialized owner by value. */
typedef struct RageTrackTextureSnapshot {
    RageTrackTextureGeneration *generation;
} RageTrackTextureSnapshot;

/* Copy a complete native-word VRAM image; return nonzero on success. */
typedef int (*RageTrackReadVram)(void *context, uint16_t *words, size_t count);

/* Source pointers are borrowed only during Acquire, never retained. Each
 * shadow row is tightly packed; nonzero shadowPages[row] denotes bank 1. */
typedef struct RageTrackTextureSource {
    RageTrackReadVram read;
    void *context;
    const void *shadowRows;
    size_t shadowBytes;
    const uint8_t *shadowPages;
    size_t shadowPageCount;
} RageTrackTextureSource;

/* page=-1 requests the unmodified snapshot without reconstructing track banks.
 * Each bank is a complete immutable VRAM image. Returned pixels are borrowed
 * until acquisition of a DIFFERENT revision or release on this owner; selecting
 * another bank or reacquiring the same revision does not mutate prior views.
 * A revision change invalidates the old content, even when the new read fails.
 * Buffers are reused only when no retained generation references exist.
 * Retained handles below extend lifetime independently of this owner. */
const uint16_t *TrackTextureSnapshotAcquire(RageTrackTextureSnapshot *snapshot,
    uint64_t revision, const RageTrackTextureSource *source, int page);
const uint16_t *TrackTextureSnapshotSelectPage(
    RageTrackTextureSnapshot *snapshot, int page);
void TrackTextureSnapshotRelease(RageTrackTextureSnapshot *snapshot);
/* Single-threaded explicit retention. A retained generation and its published
 * pixels survive owner replacement/release. Release each retained reference
 * once; no game globals or source pointers are retained. */
RageTrackTextureGeneration *TrackTextureSnapshotRetain(const RageTrackTextureSnapshot *snapshot);
const uint16_t *TrackTextureGenerationPixels(const RageTrackTextureGeneration *generation, int page);
void TrackTextureGenerationRelease(RageTrackTextureGeneration *generation);
uint64_t TrackTextureGenerationRevision(const RageTrackTextureGeneration *generation);

#endif
