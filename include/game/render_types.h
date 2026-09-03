#ifndef GAME_RENDER_TYPES_H
#define GAME_RENDER_TYPES_H

#include "common.h"
#include "game/vector.h"
#include "psyq/gpu.h"

typedef s32 RaceHudPacketOffset;
/* PSY-Z stores a native pointer and packet length in each ordering-table
 * entry; retail PSY-Q stores the packed 32-bit link in one word. */
#ifdef __psyz
typedef struct GameOrderingTableEntry {
    u_long tag;
    u_long len;
} GameOrderingTableEntry;
#else
typedef u_long GameOrderingTableEntry;
#endif

typedef struct VisibleTerrainCell {
    s32 x;
    s32 y;
    s32 z;
    s32 cellIndex;
} VisibleTerrainCell;
_Static_assert(sizeof(VisibleTerrainCell) == 16,
               "VisibleTerrainCell must match the renderer ABI");

typedef union RenderBufferAddress {
    RaceHudPacketOffset hudPacketOffset;
    s32 value;
    u8 *bytes;
    u8 **packetLink;
    void **pointerLink;
    void *pointer;
    DrawPacket *drawPacket;
    SPRT *sprite;
    SPRT_8 *sprite8;
    TILE *tile;
    LINE_F2 *lineF2;
    LINE_F3 *lineF3;
    LINE_G2 *lineG2;
    POLY_F3 *polyF3;
    POLY_F4 *polyF4;
    POLY_FT4 *polyFT4;
    POLY_G4 *polyG4;
    CVec *color;
} RenderBufferAddress;

#endif
