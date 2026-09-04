#ifndef GAME_TERRAIN_INTERNAL_H
#define GAME_TERRAIN_INTERNAL_H

#include "common.h"
#include "game/environment.h"

typedef union SkyUV {
    struct {
        u8 u;
        u8 v;
    } bytes;
    u16 packed;
} SkyUV;

typedef struct SkyTileUV {
    SkyUV corner[4];
} SkyTileUV;

enum {
    SKY_TILE_COUNT = 8,
};
extern SkyTileUV g_SkyTileUV[SKY_TILE_COUNT];

static inline s32 TerrainPrimitiveStride(s32 primitive) {
    switch (primitive) {
    case 0:
    case 2:
    case 3:
        return 0x20;
    case 1:
    case 4:
    case 5:
        return 0x24;
    default:
        return 0;
    }
}

void DrawTerrainCellsInRange(s32 nearDepth, s32 farDepth);

#endif
