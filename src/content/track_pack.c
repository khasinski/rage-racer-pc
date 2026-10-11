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
