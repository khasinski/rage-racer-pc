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

typedef struct Placement {
    int16_t model, yaw;
    int32_t x, y, z;
    uint32_t flags;
} Placement;
typedef struct Placements {
    const unsigned char *entries;
    uint32_t count;
} Placements;
/* Model -1 disables an entry. Known flags occupy the low four bits.
 * Inputs may be unaligned; failure leaves outputs and writable bytes intact. */
int PlacementsOpen(const void *data, size_t size, uint32_t models,
                   Placements *placements);
int PlacementRead(const Placements *placements, uint32_t index, Placement *out);
int PlacementWrite(void *data, size_t size, uint32_t models, uint32_t index,
                   const Placement *value);

#endif
