#include "track_pack.h"
#include <stdint.h>
#include <string.h>

static uint32_t Read32(const unsigned char *p) {
    return (uint32_t)p[0] | (uint32_t)p[1] << 8 |
           (uint32_t)p[2] << 16 | (uint32_t)p[3] << 24;
}

int TrackPackOpen(const void *data, size_t size,
                  TrackBlock blocks[TRACK_PACK_BLOCK_COUNT]) {
    const unsigned char *bytes = data;
    TrackBlock decoded[TRACK_PACK_BLOCK_COUNT];
    unsigned i;
    if (!bytes || !blocks || size < TRACK_PACK_BLOCK_COUNT * 4 ||
        size > INT32_MAX) return 0;
    for (i = 0; i < TRACK_PACK_BLOCK_COUNT; i++) {
        size_t start = Read32(bytes + i * 4);
        size_t end = i + 1 < TRACK_PACK_BLOCK_COUNT
                         ? Read32(bytes + (i + 1) * 4) : size;
        if (start < TRACK_PACK_BLOCK_COUNT * 4 || end <= start || end > size)
            return 0;
        decoded[i].data = bytes + start;
        decoded[i].size = end - start;
    }
    memcpy(blocks, decoded, sizeof(decoded));
    return 1;
}

const char *TrackBlockName(unsigned index) {
    static const char *const names[TRACK_PACK_BLOCK_COUNT] = {
        "render-table", "environment-palette", "environment-timeline",
        "rivals-and-scenery", "road", "course-models", "secondary-models",
        "terrain", "object-placements", "race-events", "cameras"
    };
    return index < TRACK_PACK_BLOCK_COUNT ? names[index] : NULL;
}

static uint16_t Read16(const unsigned char *p) {
    return (uint16_t)((uint16_t)p[0] | (uint16_t)p[1] << 8);
}

int RoadRead(const Road *road, uint32_t index, RoadPoint *point) {
    const unsigned char *p;
    if (!road || !point || !road->points || index >= road->count) return 0;
    p = road->points + (size_t)index * 24;
    point->x = (int32_t)Read32(p);point->z = (int32_t)Read32(p+4);
    point->y = (int16_t)Read16(p+8);point->angle = (int16_t)Read16(p+10);
    point->pitch = (int16_t)Read16(p+12);point->crossSlope = (int16_t)Read16(p+14);
    point->leftWidth = (int16_t)Read16(p+16);point->rightWidth = (int16_t)Read16(p+18);
    point->arc = Read16(p+20);point->length = Read16(p+22);
    return 1;
}

int RoadOpen(const void *data, size_t size, Road *road) {
    const unsigned char *bytes = data;
    Road decoded;
    uint32_t i;
    if (!bytes || !road || size < 4 || size > INT32_MAX) return 0;
    decoded.count = Read32(bytes);
    if (!decoded.count || decoded.count > (size-4)/24) return 0;
    decoded.points = bytes+4;
    decoded.arcs = decoded.points+(size_t)decoded.count*24;
    decoded.arcCount = (uint32_t)((size-4-(size_t)decoded.count*24)/12);
    decoded.length = 0;
    for (i=0;i<decoded.count;i++) {
        RoadPoint point;
        unsigned mode;
        int arcIndex;
        RoadRead(&decoded,i,&point);
        mode=point.arc & 3;
        arcIndex=(int16_t)point.arc >> 4;
        if (!point.length || point.length > INT16_MAX || mode > 2 ||
            decoded.length > ((uint32_t)INT16_MAX << 8)-1-point.length ||
            (mode && (arcIndex < 0 || (uint32_t)arcIndex >= decoded.arcCount))) return 0;
        decoded.length += point.length;
    }
    *road=decoded;
    return 1;
}
