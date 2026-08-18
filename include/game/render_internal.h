#ifndef GAME_RENDER_INTERNAL_H
#define GAME_RENDER_INTERNAL_H

#include "common.h"
#include "game/camera_types.h"
#include "game/render_workspace.h"
#include "game/vector.h"
#include "psyq/gpu.h"
#include "psyq/gte.h"

typedef struct FontGlyph {
    u8 u;
    u8 v;
    u16 width;
} FontGlyph;

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

/* The environment block at the head of each 0x237E8-byte frame context. */
typedef struct GameFrameEnvironmentHeader {
    DrawEnv draw;
    DispEnv display;
    DrawEnv mirrorDraw;
} GameFrameEnvironmentHeader;

#ifndef __psyz
typedef char GameFrameEnvironmentHeaderSizeCheck[
    sizeof(GameFrameEnvironmentHeader) == 0xCC ? 1 : -1];
#endif

#define GAME_FRAME_OT_LENGTH 0x2C0
#ifdef __psyz
/* Native packets carry pointer-sized OT links and are substantially larger
 * than their packed PS1 counterparts. The Grand Prix terrain subdivision can
 * emit about 20k packets in one frame, including paired texture-window state. */
#define GAME_FRAME_PRIMITIVE_BUFFER_SIZE 0x800000
#else
#define GAME_FRAME_PRIMITIVE_BUFFER_SIZE 0x22000
#endif

#ifndef __psyz
/* PSY-Z's libgpu.h provides the host OT_TYPE (a pointer-sized tag pair);
 * retail ordering tables hold packed 32-bit links. */
#define OT_TYPE u_long
#endif

typedef struct RaceHudPackets {
    DrawPacket tachometerDrawModes[2];
    SPRT tachometerFace;
    SPRT lapTimes[6];
    SPRT labels[6];
} RaceHudPackets;

#ifndef __psyz
typedef char RaceHudPacketsSizeCheck[
    sizeof(RaceHudPackets) == 0x11C ? 1 : -1];
#endif

typedef struct GameFrameLayout {
    GameFrameEnvironmentHeader environment;
    OT_TYPE orderingTables[2][GAME_FRAME_OT_LENGTH];
    /* Native packets contain pointer-sized ordering-table links. 0x22000 was
     * sized for four-byte PS1 links and cannot hold the same command stream. */
    u8 primitiveBuffer[GAME_FRAME_PRIMITIVE_BUFFER_SIZE];
    RaceHudPackets raceHud;
} GameFrameLayout;

#ifdef __psyz
#define GAME_FRAME_CONTEXT_SIZE ((s32)sizeof(GameFrameLayout))
#else
#define GAME_FRAME_CONTEXT_SIZE 0x237E8

typedef char GameFrameLayoutSizeCheck[
    sizeof(GameFrameLayout) == GAME_FRAME_CONTEXT_SIZE ? 1 : -1];
#endif

typedef union GameFrameContext {
    GameFrameEnvironmentHeader environment;
    GameFrameLayout layout;
    u8 bytes[sizeof(GameFrameLayout)];
    volatile u8 volatileBytes[sizeof(GameFrameLayout)];
} GameFrameContext;

typedef union GameFrameContextAddress {
    u8 *bytes;
    GameFrameContext *context;
} GameFrameContextAddress;

static __inline__ GameFrameContext *GetGameFrameContext(u8 *bytes) {
    GameFrameContextAddress address;

    address.bytes = bytes;
    return address.context;
}

typedef union ScreenOffset {
    s32 value;
    u16 displayValue;
} ScreenOffset;

extern Matrix g_MirrorViewMatrix;
extern Matrix g_SceneLightMatrix;
extern TrackRenderTable *g_TrackRenderTable;
extern FontGlyph g_SmallFontGlyphs[];
extern FontGlyph g_LargeFontGlyphs[];
extern CameraKey g_CameraPath[];
extern u32 g_MainVisibleCellMask[];
extern Vec4 g_MainVisibleCellList[];
extern u32 *g_VisibleCellMask;
extern Vec4 *g_VisibleCellList;
extern CameraViewMode g_CameraViewMode;
extern s16 g_AtanTable[];
extern u8 *g_DrawBuffer;

static __inline__ OT_TYPE *GamePrimaryOrderingTable(s32 depth) {
    return &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[0][depth];
}
static __inline__ OT_TYPE *GameSecondaryOrderingTable(s32 depth) {
    return &GetGameFrameContext(g_DrawBuffer)->layout.orderingTables[1][depth];
}
extern GameFrameContext g_FrameContexts[2];
extern ScreenOffset g_ScreenOffsetX;
extern ScreenOffset g_ScreenOffsetY;
extern volatile s32 g_FrameParity;
extern u8 g_Font8x8Cells[];
extern u8 g_DrawModeEnv[];
extern u8 g_PropFontCells[0x80];
#define g_PropFontU g_PropFontCells
#define g_PropFontV (g_PropFontCells + 1)
extern u8 g_WordFontCells[40];
#define g_WordFontU g_WordFontCells
#define g_WordFontV (g_WordFontCells + 1)
#define g_WordFontWidth (g_WordFontCells + 2)
#define g_WordFontAdvance (g_WordFontCells + 3)
extern u8 g_HighFontCell[4];
#define g_HighFontU g_HighFontCell
#define g_HighFontV (g_HighFontCell + 1)
#define g_HighFontWidth (g_HighFontCell + 2)
#define g_HighFontYOffset (g_HighFontCell + 3)
extern s32 g_MenuOverlayPatternAnimOffset;
extern u8 g_MenuOverlayPatternTable[];
extern u8 g_SpriteFontCells[192];
#define g_SpriteFontU g_SpriteFontCells
#define g_SpriteFontV (g_SpriteFontCells + 1)
extern u8 g_SpriteFontWidth[];

#endif
