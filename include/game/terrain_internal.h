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

typedef struct SkyRenderScratchpad {
    u8 *packetCursor;
    OT_TYPE *orderingTable;
    s32 cameraX;
    s32 cameraY;
    s32 cameraZ;
    s32 pad14;
    s32 pitch;
    s32 yaw;
    s32 roll;
    s32 pad24[17];
    s32 mirrorFlag;
} SkyRenderScratchpad;

typedef struct SkyClipBounds {
    s16 xMinTop;
    s16 xMaxTop;
    s16 xMinBottom;
    s16 xMaxBottom;
    s16 yEdge0;
    s16 yEdge1;
    s16 yEdge2;
    s16 yEdge3;
} SkyClipBounds;

extern SkyTileUV g_SkyTileUV[];

#endif
