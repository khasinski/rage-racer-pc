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

#endif
