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

/* A ready-made SPRT description stored in the game's data tables. */
typedef struct GameSpriteDesc {
    u16 x;
    u16 y;
    u16 w;
    u16 h;
    u8 u0;
    u8 pad9;
    u8 v0;
    u8 padB;
    u16 clut;
    u8 padE[2];
    s32 semiTrans;
} GameSpriteDesc;

enum {
    TIME_ATTACK_HUD_SPRITE_COUNT = 11,
    GRAND_PRIX_HUD_SPRITE_COUNT = 12,
};

_Static_assert(sizeof(GameSpriteDesc) == 20,
               "GameSpriteDesc must match the retail data layout");

typedef struct ProportionalFontCell {
    u8 textureU;
    u8 textureV;
} ProportionalFontCell;

enum { PROPORTIONAL_FONT_CELL_COUNT = 64 };

_Static_assert(sizeof(ProportionalFontCell) == 2,
               "proportional font cell must remain a UV byte pair");

typedef struct SpriteFontCell {
    u8 textureU;
    u8 textureV;
} SpriteFontCell;

enum { SPRITE_FONT_CELL_COUNT = 96 };

_Static_assert(sizeof(SpriteFontCell) == 2,
               "sprite font cell must remain a UV byte pair");

typedef struct Font8x8Cell {
    u8 column;
    u8 row;
} Font8x8Cell;

enum { FONT_8X8_CELL_COUNT = 96 };

_Static_assert(sizeof(Font8x8Cell) == 2,
               "8x8 font cell must remain a column/row byte pair");

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
