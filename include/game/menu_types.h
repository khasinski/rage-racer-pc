#ifndef GAME_MENU_TYPES_H
#define GAME_MENU_TYPES_H

#include "common.h"
#include "game/vector.h"

typedef enum MenuScreenId {
    MENU_SCREEN_NONE = -1,
    MENU_SCREEN_LOADING = 0,
    MENU_SCREEN_COURSE_SELECT = 1,
    MENU_SCREEN_RANKING,
    MENU_SCREEN_ENTER_CAR_SELECT,
    MENU_SCREEN_CAR_SELECT,
    MENU_SCREEN_CUSTOMIZE,
    MENU_SCREEN_DESIGN_MODE,
    MENU_SCREEN_TEAM_LOGO,
    MENU_SCREEN_LOGO_SAMPLE,
    MENU_SCREEN_TEAM_NAME,
    MENU_SCREEN_PAINT_COLOR,
    MENU_SCREEN_CAR_SHOP,
    MENU_SCREEN_ENGINEER_SHOP
} MenuScreenId;

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

typedef s32 RaceRecordOffset;

typedef union RaceRecordAddress {
    RaceRecordOffset recordOffset;
#ifdef __psyz
    uintptr_t value;
#else
    s32 value;
#endif
    RaceRecord *pointer;
    s32 *wordPointer;
    u16 *halfwordPointer;
    u8 *bytePointer;
    volatile u8 *volatileBytePointer;
} RaceRecordAddress;

static __inline__ s32 *GetRaceRecordWords(RaceRecord *record) {
    RaceRecordAddress address;

    address.pointer = record;
    return address.wordPointer;
}

typedef struct TeamLogoSample {
    u16 clut[2][16];
    u16 canvas[64][16];
} TeamLogoSample;

typedef s32 TeamLogoSampleOffset;

typedef union TeamLogoSampleAddress {
    TeamLogoSampleOffset offset;
#ifdef __psyz
    uintptr_t value;
#else
    s32 value;
#endif
    void *data;
    TeamLogoSample *samplePointer;
    u16 *halfwordPointer;
} TeamLogoSampleAddress;

static __inline__ TeamLogoSample *GetTeamLogoSample(void *data) {
    TeamLogoSampleAddress address;

    address.data = data;
    return address.samplePointer;
}

typedef union TeamLogoPixelWord {
    u16 value;
    u8 bytes[2];
} TeamLogoPixelWord;

typedef union TeamLogoCanvas {
    u8 bytes[0x800];
    u16 halfwords[0x400];
    TeamLogoPixelWord pixels[0x400];
    u32 words[64][8];
} TeamLogoCanvas;

typedef union TeamLogoCanvasAddress {
#ifdef __psyz
    uintptr_t value;
#else
    s32 value;
#endif
    u8 *bytePointer;
    u32 *wordPointer;
} TeamLogoCanvasAddress;

static __inline__ u8 *GetTeamLogoCanvasBytes(u32 *words) {
    TeamLogoCanvasAddress address;

    address.wordPointer = words;
    return address.bytePointer;
}

typedef struct PaintColorTable {
    Rgb colors[18];
} PaintColorTable;

typedef union PaintColorAddress {
#ifdef __psyz
    uintptr_t value;
#else
    s32 value;
#endif
    u8 *bytes;
    Rgb *pointer;
} PaintColorAddress;

typedef s32 TeamLogoCoordinate;
typedef s32 TeamLogoColorIndex;

typedef union TeamLogoColorSlot {
    s32 value;
    u16 low;
} TeamLogoColorSlot;

typedef union TeamLogoColorAddress {
    TeamLogoColorIndex *index;
    TeamLogoColorSlot *slot;
} TeamLogoColorAddress;

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
