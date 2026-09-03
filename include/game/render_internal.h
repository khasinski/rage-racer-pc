#ifndef GAME_RENDER_INTERNAL_H
#define GAME_RENDER_INTERNAL_H

#include "common.h"
#include "game/camera_types.h"
#include "game/integer.h"
#include "game/render_state.h"
#include "game/render_types.h"
#include "game/vector.h"
#include "psyq/gpu.h"
#include "psyq/gte.h"

typedef struct FontGlyph {
    u8 u;
    u8 v;
    u16 width;
} FontGlyph;

enum {
    FONT_GLYPH_RECORD_SIZE = 4,
    SMALL_FONT_GLYPH_COUNT = 46,
    LARGE_FONT_GLYPH_COUNT = 49,
    PRINTABLE_ASCII_FIRST = 0x20,
    PRINTABLE_ASCII_GLYPH_COUNT = 96,
    PRINTABLE_ASCII_FALLBACK_GLYPH = '?' - PRINTABLE_ASCII_FIRST,
};

static inline s32 PrintableAsciiGlyph(u8 character) {
    s32 glyph = character - PRINTABLE_ASCII_FIRST;

    return (u32)glyph < PRINTABLE_ASCII_GLYPH_COUNT
        ? glyph
        : PRINTABLE_ASCII_FALLBACK_GLYPH;
}

typedef struct CameraKey {
    s32 eyeX;
    s32 eyeY;
    s32 eyeZ;
    s32 atX;
    s32 atY;
    s32 atZ;
    s32 duration;
    s32 control;
} CameraKey;

/* The environment block at the head of each frame context. */
typedef struct GameFrameEnvironmentHeader {
    DrawEnv draw;
    DispEnv display;
    DrawEnv mirrorDraw;
} GameFrameEnvironmentHeader;


#define GAME_FRAME_OT_LENGTH 0x2C0
/* Native packets carry pointer-sized OT links and are substantially larger
 * than their packed PS1 counterparts. The Grand Prix terrain subdivision can
 * emit about 20k packets in one frame, including paired texture-window state. */
#define GAME_FRAME_PRIMITIVE_BUFFER_SIZE 0x800000


typedef struct RaceHudPackets {
    DrawPacket tachometerDrawModes[2];
    SPRT tachometerFace;
    SPRT lapTimes[6];
    SPRT labels[6];
} RaceHudPackets;


typedef struct GameFrameLayout {
    GameFrameEnvironmentHeader environment;
    GameOrderingTableEntry orderingTables[2][GAME_FRAME_OT_LENGTH];
    /* Native packets contain pointer-sized ordering-table links. 0x22000 was
     * sized for four-byte PS1 links and cannot hold the same command stream. */
    u8 primitiveBuffer[GAME_FRAME_PRIMITIVE_BUFFER_SIZE];
    RaceHudPackets raceHud;
} GameFrameLayout;

#define GAME_FRAME_CONTEXT_SIZE ((s32)sizeof(GameFrameLayout))

typedef union GameFrameContext {
    GameFrameEnvironmentHeader environment;
    GameFrameLayout layout;
    u8 bytes[sizeof(GameFrameLayout)];
} GameFrameContext;

/* Current frame work area, selected by the display swap. Emulation traces of
 * the retail game identified its two 1408-entry ordering tables; the native
 * layout keeps the same ownership while sizing packet storage for native
 * pointer-width links. */
extern GameFrameContext *g_DrawBuffer;

static inline void GameClearOrderingTable(GameOrderingTableEntry *table,
                                          s32 count) {
    ClearOTagR((void *)table, count);
}

static inline void GameDrawOrderingTable(GameOrderingTableEntry *lastEntry) {
    DrawOTag((void *)lastEntry);
}

typedef s32 ScreenOffset;

extern Matrix g_MirrorViewMatrix;
extern Matrix g_SceneLightMatrix;
extern Matrix g_SceneColorMatrix;
extern Matrix g_TrackColorMatrix;
extern Matrix g_TrackLightMatrix;
extern TrackRenderTable *g_TrackRenderTable;
extern s32 g_TrackTextureSectionLo;
extern s32 g_TrackTextureSectionHi;

static inline void ApplyTrackTextureSectionRange(void) {
    g_TrackTextureSectionLo = g_TrackRenderTable->textureSectionLo;
    g_TrackTextureSectionHi = g_TrackRenderTable->textureSectionHi;
}

void InitTrackLighting(void);

extern u8 g_SmallFontGlyphs[SMALL_FONT_GLYPH_COUNT * FONT_GLYPH_RECORD_SIZE];
extern u8 g_LargeFontGlyphs[LARGE_FONT_GLYPH_COUNT * FONT_GLYPH_RECORD_SIZE];
extern u32 g_MainVisibleCellMask[];
extern VisibleTerrainCell g_MainVisibleCellList[];
extern u32 *g_VisibleCellMask;
extern VisibleTerrainCell *g_VisibleCellList;
extern CameraViewMode g_CameraViewMode;
extern s16 g_AtanTable[];
static inline GameOrderingTableEntry *GamePrimaryOrderingTable(s32 depth) {
    return &g_DrawBuffer->layout.orderingTables[0][depth];
}
static inline GameOrderingTableEntry *GameSecondaryOrderingTable(s32 depth) {
    return &g_DrawBuffer->layout.orderingTables[1][depth];
}
extern GameFrameContext g_FrameContexts[2];
extern ScreenOffset g_ScreenOffsetX;
extern ScreenOffset g_ScreenOffsetY;
extern s32 g_FrameParity;
extern u8 g_Font8x8Cells[];
extern u8 g_DrawModeEnv[];
extern u8 g_PropFontCells[0x80];
extern u8 g_WordFontCells[40];
extern u8 g_HighFontCell[4];
extern s32 g_MenuOverlayPatternAnimOffset;
extern u8 g_MenuOverlayPatternTable[];
extern u8 g_SpriteFontCells[192];
#define g_SpriteFontU g_SpriteFontCells
#define g_SpriteFontV (g_SpriteFontCells + 1)
extern u8 g_SpriteFontWidth[];

#endif
