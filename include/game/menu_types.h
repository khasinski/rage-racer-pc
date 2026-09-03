#ifndef GAME_MENU_TYPES_H
#define GAME_MENU_TYPES_H

#include "common.h"
#include "game/vector.h"

enum {
    CLASS_RECORD_COUNT = 11,
    TEAM_LOGO_WIDTH = 64,
    TEAM_LOGO_HEIGHT = 64,
    TEAM_LOGO_EDITOR_VIEW_SIZE = 32,
    TEAM_LOGO_BITS_PER_PIXEL = 4,
    TEAM_LOGO_PIXELS_PER_WORD = 8,
    TEAM_LOGO_WORDS_PER_ROW =
        TEAM_LOGO_WIDTH / TEAM_LOGO_PIXELS_PER_WORD,
};

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
    u16 canvas[TEAM_LOGO_HEIGHT][TEAM_LOGO_WIDTH / 4];
} TeamLogoSample;

typedef union TeamLogoSampleAddress {
    const void *data;
    const TeamLogoSample *sample;
} TeamLogoSampleAddress;

static inline const TeamLogoSample *GetTeamLogoSample(const void *data) {
    TeamLogoSampleAddress address;

    address.data = data;
    return address.sample;
}

typedef union TeamLogoCanvas {
    u8 bytes[TEAM_LOGO_WIDTH * TEAM_LOGO_HEIGHT / 2];
    u16 halfwords[TEAM_LOGO_WIDTH * TEAM_LOGO_HEIGHT / 4];
    u32 words[TEAM_LOGO_HEIGHT][TEAM_LOGO_WORDS_PER_ROW];
} TeamLogoCanvas;

static inline u32 GetTeamLogoCanvasPixel(const TeamLogoCanvas *canvas, s32 x,
                                         s32 y) {
    s32 shift =
        (x % TEAM_LOGO_PIXELS_PER_WORD) * TEAM_LOGO_BITS_PER_PIXEL;

    return (canvas->words[y][x / TEAM_LOGO_PIXELS_PER_WORD] >> shift) & 0xF;
}

static inline void SetTeamLogoCanvasPixel(TeamLogoCanvas *canvas, s32 x,
                                          s32 y, u32 color) {
    s32 shift =
        (x % TEAM_LOGO_PIXELS_PER_WORD) * TEAM_LOGO_BITS_PER_PIXEL;
    u32 mask = 0xFu << shift;
    u32 *word = &canvas->words[y][x / TEAM_LOGO_PIXELS_PER_WORD];

    *word = (*word & ~mask) | ((color & 0xF) << shift);
}

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

enum { MENU_OPTION_HINT_COUNT = 7 };

/* One entry in the setup-screen hint bar. */
typedef struct OptionHintCaption {
    u8 u;
    u8 v;
    u8 width;
    u8 advance;
} OptionHintCaption;

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
