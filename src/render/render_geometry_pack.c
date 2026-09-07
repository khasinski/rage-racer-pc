#include "render_geometry_pack.h"
#include <stdlib.h>
#include <string.h>

/* The renderer's wire-to-GPU vertex has no padding to hash accidentally. */
_Static_assert(sizeof(RageNativeGpuVertex) == 56, "Update geometry identity for vertex layout changes");

void RenderGeometryPackRelease(RageNativeGeometryPack *pack) {
    if (pack == NULL) return;
    free(pack->vertices); free(pack->indices); free(pack->slots);
    memset(pack, 0, sizeof(*pack));
}

static uint32_t VertexHash(const RageNativeGpuVertex *vertex) {
    uint32_t hash = 2166136261u;
    const unsigned char *bytes = (const unsigned char *)vertex;
    for (size_t byte = 0; byte < sizeof(*vertex); byte += sizeof(uint32_t)) {
        uint32_t word;
        memcpy(&word, bytes + byte, sizeof(word));
        hash = (hash ^ word) * 16777619u;
    }
    /* Mix high float bits into low table bits (integer-valued coordinates
     * otherwise share long zero mantissa suffixes). */
    hash ^= hash >> 16;
    hash *= 0x85ebca6bu;
    hash ^= hash >> 13;
    hash *= 0xc2b2ae35u;
    return hash ^ (hash >> 16);
}

static void ResetPack(RageNativeGeometryPack *pack) {
    pack->vertexCount = pack->indexCount = 0;
    pack->residentCount = 0;
    if (pack->slots != NULL)
        memset(pack->slots, 0, (size_t)pack->capacity * 2 * sizeof(*pack->slots));
    ++pack->generation;
}

static int RetainVertex(const RageNativeGeometryRange *range, uint32_t index) {
    return range->retainMask ? range->retainMask[index] != 0 : range->retain != 0;
}

int RenderGeometryPackAppendRanges(RageNativeGeometryPack *pack,
    const RageNativeGeometryRange *ranges, uint32_t rangeCount, uint32_t limit) {
    if (pack == NULL) return 0;
    pack->indexCount = 0;
    pack->vertexCount = pack->residentCount;
    if (rangeCount && !ranges) return 0;
    uint32_t count = 0;
    for (uint32_t r = 0; r < rangeCount; ++r) {
        if (ranges[r].count > UINT32_MAX - count ||
            (ranges[r].count && (!ranges[r].vertices || ranges[r].vertices == pack->vertices))) return 0;
        count += ranges[r].count;
    }
    if (count == 0) return 1;
    if (count > limit || limit > (1u << 24)) return 0;
    if (pack->vertexCount > limit - count) ResetPack(pack);
    uint32_t needed = pack->vertexCount + count;
    if (pack->capacity < needed) {
        uint32_t capacity = 64;
        while (capacity < needed) capacity *= 2;
        RageNativeGeometryPack next = {0};
        next.vertices = malloc((size_t)capacity * sizeof(*next.vertices));
        next.indices = malloc((size_t)capacity * sizeof(*next.indices));
        next.slots = malloc((size_t)capacity * 2 * sizeof(*next.slots));
        if (next.vertices == NULL || next.indices == NULL || next.slots == NULL) {
            RenderGeometryPackRelease(&next);
            return 0;
        }
        next.capacity = capacity;
        next.vertexCount = pack->vertexCount;
        next.residentCount = pack->residentCount;
        next.generation = pack->generation;
        if (pack->vertexCount != 0)
            memcpy(next.vertices, pack->vertices, (size_t)pack->vertexCount * sizeof(*pack->vertices));
        memset(next.slots, 0, (size_t)capacity * 2 * sizeof(*next.slots));
        for (uint32_t i = 0; i < next.vertexCount; ++i) {
            uint32_t slot = VertexHash(&next.vertices[i]) & (capacity * 2 - 1);
            while (next.slots[slot] != 0) slot = (slot + 1) & (capacity * 2 - 1);
            next.slots[slot] = i + 1;
        }
        RenderGeometryPackRelease(pack);
        *pack = next;
    }
    uint32_t mask = pack->capacity * 2 - 1;
    uint32_t offset = 0;
    for (uint32_t r = 0; r < rangeCount; ++r) {
        if (!ranges[r].retainMask && !ranges[r].retain) {
            offset += ranges[r].count;
            continue;
        }
        const RageNativeGpuVertex *source = ranges[r].vertices;
        for (uint32_t i = 0; i < ranges[r].count; ++i) {
            if (!RetainVertex(&ranges[r], i)) continue;
            uint32_t slot = VertexHash(&source[i]) & mask;
            while (pack->slots[slot] != 0 &&
                   memcmp(&pack->vertices[pack->slots[slot] - 1], &source[i], sizeof(*source)) != 0)
                slot = (slot + 1) & mask;
            if (pack->slots[slot] == 0) {
                memcpy(&pack->vertices[pack->vertexCount], &source[i], sizeof(*source));
                pack->slots[slot] = ++pack->vertexCount;
            }
            pack->indices[offset + i] = pack->slots[slot] - 1;
        }
        offset += ranges[r].count;
    }
    pack->residentCount = pack->vertexCount;
    offset = 0;
    for (uint32_t r = 0; r < rangeCount; ++r) {
        if (!ranges[r].retainMask && ranges[r].retain) {
            offset += ranges[r].count;
            continue;
        }
        const RageNativeGpuVertex *source = ranges[r].vertices;
        for (uint32_t i = 0; i < ranges[r].count; ++i) {
            if (RetainVertex(&ranges[r], i)) continue;
            memcpy(&pack->vertices[pack->vertexCount], &source[i], sizeof(*source));
            pack->indices[offset + i] = pack->vertexCount++;
        }
        offset += ranges[r].count;
    }
    pack->indexCount = count;
    return 1;
}

int RenderGeometryPackAppendSelected(RageNativeGeometryPack *pack,
    const RageNativeGpuVertex *source, uint32_t count,
    const uint8_t *retainMask, uint32_t limit) {
    const RageNativeGeometryRange range = {source, count, retainMask, 1};
    return RenderGeometryPackAppendRanges(pack, &range, 1, limit);
}

int RenderGeometryPackAppend(RageNativeGeometryPack *pack,
    const RageNativeGpuVertex *source, uint32_t count, uint32_t limit) {
    return RenderGeometryPackAppendSelected(pack, source, count, NULL, limit);
}

int RenderGeometryPackBuild(RageNativeGeometryPack *pack,
    const RageNativeGpuVertex *source, uint32_t count) {
    if (pack == NULL) return 0;
    ResetPack(pack);
    return RenderGeometryPackAppend(pack, source, count, 1u << 24);
}
