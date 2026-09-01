#ifndef GAME_MENU_TYPES_H
#define GAME_MENU_TYPES_H

#include "common.h"
#include "game/vector.h"

typedef struct ScoreRecord {
    s16 place;
    u16 clears;
} ScoreRecord;

typedef struct RaceRecord {
    char driverName[8];
    s32 raceTime;
    s16 carIndex;
    s16 unused;
} RaceRecord;

typedef struct TeamLogoSample {
    u16 clut[2][16];
    u16 canvas[64][16];
} TeamLogoSample;

static inline TeamLogoSample *GetTeamLogoSample(void *data) {
    return (TeamLogoSample *)data;
}

typedef union TeamLogoCanvas {
    u8 bytes[0x800];
    u16 halfwords[0x400];
    u32 words[64][8];
} TeamLogoCanvas;

typedef struct PaintColorTable {
    Rgb colors[18];
} PaintColorTable;

typedef s32 TeamLogoCoordinate;
typedef s32 TeamLogoColorIndex;

typedef struct TeamLogoClutPos {
    s16 x;
    s16 y;
} TeamLogoClutPos;

typedef struct TeamLogoTexturePos {
    s16 x;
    u16 y;
} TeamLogoTexturePos;

typedef struct MenuLightBurstBand {
    s16 values[33];
} MenuLightBurstBand;

enum MenuPalette {
    MENU_PAINT_COLOR_COUNT = 18
};

/* 6x6 mask selecting the DESIGN MODE script cells to draw. */
typedef struct DesignModeCellMask {
    u8 cells[6][6];
} DesignModeCellMask;

/* Screen-space output produced while flipping the course card. */
typedef union MenuProjectedVertex {
    struct {
        s16 x;
        s16 y;
        s16 z;
        s16 pad;
    } position;
    s16 components[4];
} MenuProjectedVertex;

/* Texture coordinates for the four glyph cells of a NeGcon setup panel. */
typedef struct NegconUvTemplate {
    u8 uv[8];
} NegconUvTemplate;

typedef struct ClassRecordSprite {
    u8 u1;
    u8 v1;
    u8 u2;
    u8 v2;
    u16 clut1;
    u16 clut2;
    u16 clut3;
    u16 unused;
} ClassRecordSprite;

#endif
