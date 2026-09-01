#ifndef GAME_TERRAIN_INTERNAL_H
#define GAME_TERRAIN_INTERNAL_H

#include "common.h"
#include "game/environment.h"
#include "psyq/gpu.h"

typedef union SkyUV {
    struct {
        u8 u;
        u8 v;
    } bytes;
    u16 packed;
} SkyUV;

typedef union GpuUvAddress {
    u8 *bytes;
    u16 *packed;
} GpuUvAddress;

typedef struct SkyTileUV {
    SkyUV corner[4];
} SkyTileUV;

extern SkyTileUV g_SkyTileUV[];

#endif
