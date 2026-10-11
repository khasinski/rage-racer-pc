#ifndef TRACK_PACK_H
#define TRACK_PACK_H

#include <stddef.h>
#include <stdint.h>

enum { TRACK_PACK_BLOCK_COUNT = 11 };
typedef struct TrackBlock {
    const unsigned char *data;
    size_t size;
} TrackBlock;

/* Borrowed spans into original bytes. Failure leaves output untouched.
 * This checks the container; each block still requires its typed validator. */
int TrackPackOpen(const void *data, size_t size,
                  TrackBlock blocks[TRACK_PACK_BLOCK_COUNT]);
const char *TrackBlockName(unsigned index);

typedef struct RoadPoint {
    int32_t x, z;
    int16_t y, angle, pitch, crossSlope, leftWidth, rightWidth;
    uint16_t arc, length;
} RoadPoint;
typedef struct Road {
    const unsigned char *points, *arcs;
    uint32_t count, arcCount, length;
} Road;
/* Validates every point and arc reference. Retains unknown trailing bytes. */
int RoadOpen(const void *data, size_t size, Road *road);
int RoadRead(const Road *road, uint32_t index, RoadPoint *point);

#endif
