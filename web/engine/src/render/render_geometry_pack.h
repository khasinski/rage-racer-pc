#ifndef RAGE_RENDER_GEOMETRY_PACK_H
#define RAGE_RENDER_GEOMETRY_PACK_H

#include "render_native_vertex.h"

/* Shared vertex payload, ordered indices. Spans retain their original
 * first/count offsets, interpreted as indices by the GPU. No reordering. */
typedef struct RageNativeGeometryPack {
    RageNativeGpuVertex *vertices;
    uint32_t *indices;
    uint32_t *slots;
    uint32_t capacity, vertexCount, indexCount, residentCount;
    uint64_t generation;
} RageNativeGeometryPack;

/* Initialize with {0}. Input must not alias owned storage. Equality is exact
 * byte identity, including every shading/UV/depth attribute, never position
 * alone. Failure clears published counts; allocated storage stays owned. */
int RenderGeometryPackBuild(RageNativeGeometryPack *pack,
    const RageNativeGpuVertex *source, uint32_t count);
/* Retain exact payload identities between frames. The worst-case reservation
 * count must fit limit; reset before a frame when the remaining budget cannot
 * hold count new vertices. A generation change requires a full GPU upload.
 * Otherwise existing vertex indices are stable, even when CPU storage grows.
 * Failure publishes no indices; previously owned storage remains releasable. */
int RenderGeometryPackAppend(RageNativeGeometryPack *pack,
    const RageNativeGpuVertex *source, uint32_t count, uint32_t limit);
/* A zero mask entry is transient: no hash lookup/insertion, appended after
 * the resident prefix and replaced next frame. NULL retains every vertex.
 * Uploads must preserve only residentCount vertices, not the transient tail. */
int RenderGeometryPackAppendSelected(RageNativeGeometryPack *pack,
    const RageNativeGpuVertex *source, uint32_t count,
    const uint8_t *retainMask, uint32_t limit);
void RenderGeometryPackRelease(RageNativeGeometryPack *pack);

/* Borrowed input ranges, concatenated in order without intermediate staging.
 * A mask overrides retain for individual vertices. Inputs must not alias pack
 * storage; all ranges are validated before insertion. Same residency and
 * failure contract as AppendSelected. */
typedef struct RageNativeGeometryRange {
    const RageNativeGpuVertex *vertices;
    uint32_t count;
    const uint8_t *retainMask;
    int retain;
} RageNativeGeometryRange;
int RenderGeometryPackAppendRanges(RageNativeGeometryPack *pack,
    const RageNativeGeometryRange *ranges, uint32_t rangeCount, uint32_t limit);

#endif
