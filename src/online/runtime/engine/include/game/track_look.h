#ifndef GAME_TRACK_LOOK_H
#define GAME_TRACK_LOOK_H
#include "game/car_asset.h"

typedef struct TrackLook {
    s32 textureSectionLo, textureSectionHi, environmentStart;
    CarModelRenderParams models[CAR_MODEL_BANK_ENTRY_COUNT];
} TrackLook;

typedef struct RivalLook {
    CarShape shape;
    u32 bodyMesh;
    u8 palette;
} RivalLook;

/* Copies placement data; input need not remain alive or be aligned.
 * Invalid input preserves the destination. */
int ReadTrackLook(const void *data, size_t size, TrackLook *look);
/* Course is the physical 0..3 slot, not the class/reverse menu index.
 * Checks that the selected body, wheels and far LOD exist in the bank. */
int ReadRivalLook(const TrackLook *track, s32 course, s32 slot,
                  s32 modelCount, RivalLook *look);
#endif
