#ifndef GAME_RENDER_H
#define GAME_RENDER_H

#include "common.h"
#include "game/angle.h"
#include "game/camera_types.h"
#include "game/environment.h"
#include "game/render_types.h"
#include "game/vector.h"

#include "game/render_state.h"
#include "psyq/gpu.h"
#include "psyq/gte.h"

enum {
    TRACK_TEXTURE_PAGE_ROW_COUNT = 256
};

/*
 * One entry of a timed draw script. `type` picks the primitive and `time` is
 * when it starts; the two pointers split the description in two, which is what
 * lets several entries share one of them: `shape` is the static side (size,
 * texel origin, draw flags) and `motion` the animated side (the interpolation
 * limit, start position, per-step delta, colour and CLUT).
 */
typedef union TimedDrawArgument {
    s32 value;
    void *pointer;
    u8 *bytes;
    s32 *words;
    struct ScriptedSpriteShape *spriteShape;
    struct ScriptedSpriteMotion *spriteMotion;
    struct ScriptedLineShape *lineShape;
    struct ScriptedLineMotion *lineMotion;
    struct ScriptedTriangleShape *triangleShape;
    struct ScriptedTriangleMotion *triangleMotion;
    struct ScriptedQuadShape *quadShape;
    struct ScriptedQuadMotion *quadMotion;
} TimedDrawArgument;

typedef struct ScriptedSpriteShape {
    s16 width;
    s16 height;
    u8 u;
    u8 v;
    u8 flags;
    u8 alpha;
} ScriptedSpriteShape;

typedef struct ScriptedSpriteMotion {
    s32 limit;
    s16 x;
    s16 y;
    u16 clut;
    u8 r;
    u8 g;
    u8 b;
    u8 padD[3];
    s32 packedVelocity;
} ScriptedSpriteMotion;

typedef struct ScriptedLineShape {
    u8 r;
    u8 g;
    u8 b;
    u8 flags;
} ScriptedLineShape;

typedef struct ScriptedLineMotion {
    s32 limit;
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    s32 packedVelocity0;
    s32 packedVelocity1;
} ScriptedLineMotion;

typedef struct ScriptedTriangleShape {
    u16 x1;
    u16 y1;
    u16 x2;
    u16 y2;
    u8 r;
    u8 g;
    u8 b;
    u8 flags;
} ScriptedTriangleShape;

typedef struct ScriptedTriangleMotion {
    s32 limit;
    s16 x;
    s16 y;
    s32 packedVelocity;
} ScriptedTriangleMotion;

typedef struct ScriptedQuadShape {
    u8 u0;
    u8 v0;
    u8 u1;
    u8 v1;
    u8 u2;
    u8 v2;
    u8 u3;
    u8 v3;
    u16 clut;
    u8 r;
    u8 g;
    u8 b;
    u8 flags;
    u8 alpha;
} ScriptedQuadShape;

typedef struct ScriptedQuadMotion {
    s32 limit;
    s16 x;
    s16 y;
    s16 width;
    s16 height;
    s32 packedVelocity;
    s32 packedSizeVelocity;
} ScriptedQuadMotion;

typedef struct TimedDrawCommand {
    s16 time;
    s16 type;
    TimedDrawArgument shape;
    TimedDrawArgument motion;
} TimedDrawCommand;

/* Where a script's clock stands this frame: the progress its commands are
 * drawn at, which is before a step is applied, and whether it has run out. */
typedef struct TimedDrawScriptTick {
    s32 drawAt;
    s32 finished;
} TimedDrawScriptTick;

TimedDrawScriptTick AdvanceTimedDrawScript(
    const TimedDrawCommand *commands, s32 *progress, s32 step);
void DrawTimedDrawScript(const TimedDrawCommand *commands, s32 progress);

/* A ready-made SPRT description stored in the game's data tables.
 * BuildSpriteFromDesc expands it into a render-state SPRT. */
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

extern GameSpriteDesc g_TachoNeedleSprite;
void BuildSpriteFromDesc(SPRT *sprite, const GameSpriteDesc *desc);
enum {
    FADING_MENU_ROW_COUNT = 5,
};
extern TimedDrawCommand g_MenuRowScript[];

/* One display buffer, which is what InitRenderState sets that rectangle to.
 * See render/display_setup.c: the 240 mode is "two 320x240 buffers stacked at
 * y=0 / y=0xF0" and sets the GTE projection with SetGeomScreen(0x140). The
 * 480 mode is the pair treated as one, which is the clip rectangle's y1 = 0x1E0 that
 * render/display_setup.c writes. */
#define SCREEN_WIDTH   0x140
#define SCREEN_HEIGHT  0xF0

/*
 * Prefix used by the renderer for car-like objects. The full camera-car
 * storage is declared as GameCarRuntime in game/car.h; every field below has
 * the same offset in that canonical type.
 */
typedef union CameraLookAt {
    struct {
        s32 eyeX;
        s32 eyeY;
        s32 eyeZ;
        s32 targetX;
        s32 targetY;
        s32 targetZ;
        s32 reserved[2];
    } fields;
    s32 words[8];
} CameraLookAt;

typedef struct GameRenderObject {
    s32 x;          /* 0x00 */
    s32 y;          /* 0x04 */
    s32 z;          /* 0x08 */
    s32 positionW;
    s32 motionX;
    s32 motionY;
    s32 motionZ;
    s32 field_1C;
    s32 bodyPitch; /* 0x20 */
    s32 bodyYaw;   /* 0x24 */
    s32 bodyRoll;  /* 0x28 */
    s32 bodyRotationW;
    s32 trackPointIndex;
    s32 trackLateralOffset;
    s32 segmentFraction;
    s32 normalizedLateralOffset;
    s32 reserved40;
    s32 steeringAngle; /* 0x44 */
    s32 wheelRotation; /* 0x48, 12-bit phase plus high-speed bit 0x1000 */
    s32 field_4C;
    s32 modelPitch; /* 0x50 */
    s32 modelYaw;   /* 0x54 */
    s32 modelRoll;  /* 0x58 */
    s32 field_5C;
    s32 modelY;     /* 0x60 secondary model origin, normally copied from y */
    s32 bodyRollVelocity; /* 0x64, damped and accumulated into bodyRoll */
    s32 progressA;
    s32 progressB;
    s32 trackProgress;   /* 0x70 */
    s32 previousTrackProgress;
    s16 trackSection;
    s16 field_7A;
    s16 velocityX;
    s16 velocityZ;
    s16 motionActive;
    u16 motionTimer;
    s16 motionMode;
    s16 motionModeTimer;
    s16 motionValue;
    s16 collisionFlag;
    s16 tiltCounter;
    s16 reserved8E;
    s16 verticalPitch;
    s16 bodyKickOffset;
    s16 verticalRoll;
    s16 reserved96;
    s16 verticalMotionState;
    u16 verticalMotionTimer;
    s16 verticalMotionRate;
    s16 verticalTargetY;
    s32 headingAngle;
    s32 speed;
    s32 acceleration;
    s16 activeFlag;
    s16 modelIndex;
    s32 initializedFlag;
    s32 trackHeadingPacked;
    s16 facingBackwards;
    u8 padBA[2];
    s32 aiEnabled;
    s32 field_C0;
    s32 reservedC4;
    s32 worldVelocityX;
    s32 reservedCC;
    s32 worldVelocityZ;
    s32 field_D4;
    s32 reservedD8;
    s32 reservedDC;
    s32 reservedE0;
    s32 renderDepth;   /* 0xE4 */
} GameRenderObject;

void DrawCar(GameRenderObject *object);

typedef struct GameRenderSourcePoint {
    u16 x;
    u8 pad2[2];
    u16 y;
    u8 pad6[2];
    u16 z;
    u8 padA[0x16];
    u16 bodyPitch;
    u8 pad22[2];
    u16 bodyYaw;
    u8 pad26[2];
    u16 bodyRoll;
    u8 pad2A[6];
    u32 trackPointIndex;
    u8 pad34[0x10];
    u16 steeringAngle;
    u8 pad46[2];
    u16 wheelRotation;
    u8 pad4A[0x16];
    u16 modelY;
    u8 pad62[0x2A];
    s16 tiltCounter;
    u8 pad8E[0x20];
    s16 modelIndex;
} GameRenderSourcePoint;

/*
 * Rotation-matrix builders. Each fills only the 3x3 part of `mtx` with a
 * rotation about one axis by a 12-bit angle (0x1000 = one turn), leaving the
 * translation alone; sin/cos come from rsin/rcos
 * and 1.0 is 0x1000.
 */
void BuildRotMatrixZ(void *mtx, s32 angle);
void BuildRotMatrixY(void *mtx, s32 angle);
void BuildRotMatrixX(void *mtx, s32 angle);
/*
 * Composes Y*X*Z from the render state's camera angles into its matrix and
 * installs it with SetRotMatrix;
 * g_MirrorViewMatrix gets the same matrix pre-multiplied by a 180-degree Y turn.
 */
void SetCameraRotMatrix(void);
/*
 * atan2 over the arctangent table g_AtanTable, in 12-bit angle units
 * (0x400 = 90 degrees). Argument order is (x, y), the reverse of C's atan2:
 * Atan2(0, +y) is 0x400.
 */
s32 Atan2(s32 x, s32 y);
/* (s32 a, s32 b); left unprototyped because UpdateCarDrivetrain calls it with two
 * extra arguments that the original left live in a2/a3. */

/*
 * Sets up both environments for the frame and clears to (r, g, b):
 * 240 = two 320x240 buffers stacked at y=0 / y=0xF0, 480 = one 320x480 pair.
 * Both also set the GTE projection (SetGeomOffset / SetGeomScreen 0x140) and
 * the render state's lower vertical clip boundary to the display height.
 */
void SetupDisplay240(s32 r, s32 g, s32 b);
void SetupDisplay480(s32 r, s32 g, s32 b);
void InitRenderState(s32 otShift);
void DrawFullscreenFadeTile(s32 color, s32 tpage);
void DrawFullscreenFadeTile480(s32 color, s32 tpage);
void RequestTrackTexturePage(s32 trackSection);
s32 TrackTexturePageForSection(s32 trackSection);
void UpdateCamera(CameraViewMode cameraModeSel, GameRenderObject *car);
void DrawPlayerCarModel(GameRenderObject *object);
void DrawTimeValue(s32 x, s32 y, s32 value, s32 color, s32 divisor);

/*
 * Model banks. SelectModelBank points the render state's bank cursor
 * (0x1F800050/54/58) and g_ModelBankCount at entry `index` of the registered
 * bank table g_ModelBanks; SubmitModel then walks model `index` of that bank
 * into the render state's ordering table. The Course variants use the separate
 * course object bank at 0x1F800048 (size g_CourseModelCount); ...2 is the same
 * routine running the second opcode table (jtbl_8007DA64, not jtbl_8007DA54).
 * All four are entry points of the hand-written GTE engine, so `ctx` is always
 * the render state.
 */
void SelectModelBank(s32 index);
void SubmitModel(void *ctx, s32 index);
void SubmitCourseModel(void *ctx, s32 index);
void SubmitCourseModel2(void *ctx, s32 index);

/* Per-frame draw loop over the world object array g_CourseObjects: culls each entry
 * against the visibility bitmask, transforms it and submits its model. */

/*
 * Per-frame environment step: advances the course's environment command script
 * (g_EnvScriptCursor), cross-fades the 16-entry sky/fog CLUT between
 * g_EnvironmentModePrev and g_EnvironmentMode into VRAM at (0xE0, 0x1E6), and
 * updates the GTE far colour and fog distance. Does not draw anything.
 */
void UpdateEnvironment(void);

/*
 * 2D/HUD primitive emitters. All of them pack the primitive at the render state's
 * cursor `*(u8 **)0x1F800000`, bump that cursor past the primitive and link it
 * into the ordering table `ot` with AddPrim. Sprites and textured quads turn a
 * linear CLUT index into VRAM clut coordinates (20 cluts per row, first row at
 * y = 0x1E0). Where a `drawMode` argument exists, 0xFF means opaque; anything
 * else enables semi-transparency and appends a GPU draw-mode packet carrying
 * that value. For sprite and flat-polygon `flags`, bit 0x80 means that the
 * caller already configured the draw mode; otherwise the low 16 bits are
 * queued after the primitive.
 */
void DrawSprite(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 u0, u16 v0,
                u8 r, u8 g, u8 b, u16 clutX, s32 shadeTex, s32 semiTrans,
                u32 flags);
void DrawFlatTriangle(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                      u16 y2, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags);
void DrawFlatQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2, u16 y2,
                  u16 x3, u16 y3, u8 r, u8 g, u8 b, s32 semiTrans, u32 flags);
/* POLY_FT4: four xy/uv pairs, flat rgb, tpage and a CLUT index as depth key. */
void GameDrawTexturedQuad(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, u16 y1, u16 x2,
                          u16 y2, u16 x3, u16 y3, u8 u0, u8 v0, u8 u1, u8 v1,
                          u8 u2, u8 v2, u8 u3, u8 v3, u8 r, u8 g, u8 b,
                          u16 clutIndex, s32 shadeTex, s32 semiTrans,
                          u16 tpage);
/* TILE: solid rectangle at (x, y) sized (w, h). */
void DrawSolidRect(
    GameOrderingTableEntry *ot,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 r,
    s32 g,
    s32 b,
    s32 drawMode);
void DrawLine(
    GameOrderingTableEntry *ot,
    s32 x0,
    s32 y0,
    s32 x1,
    s32 y1,
    s32 r,
    s32 g,
    s32 b,
    s32 drawMode);
/* LINE_F3: flat-shaded 3-point polyline. */
void DrawPolyLine3(GameOrderingTableEntry *ot, s16 x0, s16 y0, s16 x1, s16 y1, s16 x2, s16 y2,
                   u8 r, u8 g, u8 b, u8 drawMode);
/* LINE_G2: line interpolating rgb0 -> rgb1. */
void DrawGradientLine(
    GameOrderingTableEntry *ot,
    s32 x0,
    s32 y0,
    s32 x1,
    u16 y1,
    u8 r0,
    u8 g0,
    u8 b0,
    u8 r1,
    u8 g1,
    u8 b1,
    u8 drawMode);
/* Two-pixel-thick rectangle border, built from six DrawLine calls. */
void DrawRectOutline(void *buf, s32 xa, s32 ya, s32 w, s32 h, u8 r, u8 g,
                     u8 b, u8 code);
/* Clips (x, y, w, h) to the 320x480 frame and queues a SetDrawArea packet. */
void SetDrawClipRect(
    GameOrderingTableEntry *ot,
    s32 x,
    s32 y,
    s32 w,
    s32 h);

/*
 * Text and number output, both built on DrawSprite. The two fonts differ
 * only in cell size and glyph table: small = 6x12 (g_SmallFontGlyphs), large = 8x16
 * (g_LargeFontGlyphs). Bit 0x80 of `flags` selects fixed-width cells instead of the
 * per-glyph widths in the table.
 */
void DrawSmallText(s32 x, s16 y, const char *text, u8 red, u8 green, u8 blue,
                   u16 clut, s32 flags);
void DrawLargeText(s32 x, s16 y, const char *text, u8 red, u8 green, u8 blue,
                   u16 clut, s32 flags);
enum DrawNumberFlags {
    DRAW_NUMBER_LARGE_DIGITS = 1 << 0,
    DRAW_NUMBER_TEN_DIGIT_FIELD = 1 << 1,
    DRAW_NUMBER_ALT_DIGIT_ATLAS = 1 << 2,
    DRAW_NUMBER_OVERLAY_LAYER = 1 << 3,
};

/* Draws an unsigned decimal with leading zeros omitted. TEN_DIGIT_FIELD keeps
 * their horizontal space, while the return value remains the sprite count. */
s32 GameDrawNumber(s32 x, s16 y, s32 flags, u32 value, u8 red, u8 green,
                   u8 blue,
                   u16 clut, u8 primitiveCount);
/* Blits an 8x6 bit pattern from g_MenuOverlayPatternTable as 4x8 blocks; negative argument
 * animates through the table. */
void DrawBitPatternOverlay(s32 pattern);

/*
 * Full-screen fade level, 0..0x100, passed straight to
 * DrawFullscreenFadeTile and DrawRaceEndBanner. Each frame the
 * owning scene adds g_FadeStep and clamps back into range, so a scene fades in
 * or out just by setting the step.
 */
extern s32 g_FadeLevel;

/* Per-frame delta added to g_FadeLevel; negative fades out, positive fades in. */
extern s32 g_FadeStep;

/*
 * Timed draw script: a table of {time, type, shape, motion} entries replayed
 * against a progress counter, terminated by time < 0. Element types 0/1/9 draw
 * a sprite, 10/19 a line, 20/29 a triangle and 30/39 a textured quad; the +9
 * variants are skipped while g_MenuAltLayout is set. Each element interpolates its
 * position from a packed s16 velocity pair by (elapsed * velocity) >> 5.
 * Returns 1 once the progress counter has reached the terminator's limit.
 */
s32 RunTimedDrawScript(
    const TimedDrawCommand *commands,
    s32 *progress,
    s32 step);

/*
 * The progress counter the menu/UI screens hand to RunTimedDrawScript. Each
 * screen resets it to 0 on entry, then passes &g_UiScriptProgress with step +1
 * while opening and -1 while closing, and treats `<= 0` as "the close animation
 * has finished". It is also fed to DrawFadingMenuSprites as the elapsed time of the
 * panel it is animating.
 */
extern s32 g_UiScriptProgress;

/*
 * A second, independent RunTimedDrawScript progress counter. The menu
 * screens animate two script layers at once and step them separately - see
 * UpdateCourseSelectScreen, which drives &g_UiScriptProgress against one command table and
 * &g_UiScriptProgress2 against the screen's own tables in the same frame. Which
 * layer is "background" and which is "foreground" is not settled, hence the
 * neutral name (cf. g_PadPressedRepeat / g_PadPressed).
 */
extern s32 g_UiScriptProgress2;
void DrawScriptedSprite(
    s32 elapsed,
    const ScriptedSpriteShape *style,
    const ScriptedSpriteMotion *record,
    s32 useAlpha);
void DrawScriptedLine(s32 elapsed, const ScriptedLineShape *style,
                      const ScriptedLineMotion *record);
void DrawScriptedTriangle(s32 elapsed, const ScriptedTriangleShape *shape,
                          const ScriptedTriangleMotion *motion);
void DrawScriptedQuad(s32 elapsed, const ScriptedQuadShape *shape,
                      const ScriptedQuadMotion *motion);

/*
 * Low-level packet builders from the first 0x3900 bytes of .text (the boot /
 * controller-setup / top-level block). Unlike the GameDraw* emitters above,
 * these take the ordering table AND the packet cursor explicitly and return
 * the advanced cursor, so a caller can build a run of packets and write the
 * render-state cursor back once at the end.
 *   "Shaded" = takes an extra intensity written to r = g = b (SetShadeTex is
 *              NOT applied, so the texel is modulated).
 *   "Trans"  = calls SetSemiTrans(prim, 1).
 * Plain GameQueueSprite/GameQueueTexturedRect apply SetShadeTex(prim, 1), i.e.
 * the texture is drawn raw.
 */
/* SPRT, 20 bytes. */
u8 *GameQueueShadedSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 w, s32 h,
                          s32 u, s32 v, s32 clutIndex, s32 intensity);
u8 *GameQueueShadedSpriteTrans(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex,
    s32 intensity);
u8 *GameQueueSpriteTrans(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex);
u8 *DrawShadowedTile(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y);
/* Controller-configuration widgets and their complete pad diagrams. */
u8 *DrawLeftArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                  s32 pulse);
u8 *DrawRightArrow(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                   s32 pulse);
u8 *DrawPadConfigSelector(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y,
                          s32 selection);
u8 *DrawPadConfigLabels(GameOrderingTableEntry *ot, u8 *prim,
                        const u8 *labelRow);
u8 *DrawPadConfigCallouts(GameOrderingTableEntry *ot, u8 *prim,
                          const u8 *labelRow, const u8 *buttonRow);
u8 *DrawPadConfigDiagram(GameOrderingTableEntry *ot, u8 *prim);
u8 *DrawNegconConfigDiagram(GameOrderingTableEntry *ot, u8 *prim);
/* TILE, 16 bytes. */
u8 *GameQueueTileTrans(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 r,
    s32 g,
    s32 b);
/* LINE_F2, 16 bytes. */
u8 *GameQueueLine(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x0,
    s32 y0,
    s32 x1,
    s32 y1,
    s32 r,
    s32 g,
    s32 b);
/*
 * POLY_FT4, 40 bytes, built as an axis-aligned rectangle at (x, y) sized
 * (w, h). A negative w or h flips the u or v axis instead of the geometry.
 * GameQueueShadedTexturedRect derives the uv extent from w/h;
 * GameQueueTexturedRect takes uSpan/vSpan separately.
 */
u8 *GameQueueShadedTexturedRect(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 clutIndex,
    s32 tpage,
    s32 intensity);

u8 *GameQueueSprite(GameOrderingTableEntry *ot, u8 *prim, s32 x, s32 y, s32 w, s32 h, s32 u,
                    s32 v, s32 clutIndex);
u8 *GameQueueTexturedRect(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 uSpan,
    s32 vSpan,
    s32 clutIndex,
    s32 tpage);

typedef enum GameTexturePacketKind {
    GAME_TEXTURE_PACKET_SPRT,
    GAME_TEXTURE_PACKET_FT4
} GameTexturePacketKind;

static inline u8 *GameQueueTexturePacketWide(
    GameOrderingTableEntry *ot,
    u8 *prim,
    s32 x,
    s32 y,
    s32 w,
    s32 h,
    s32 u,
    s32 v,
    s32 uSpan,
    s32 vSpan,
    s32 clutIndex,
    s32 tpage,
    GameTexturePacketKind kind)
{
    if (kind == GAME_TEXTURE_PACKET_SPRT) {
        return GameQueueSprite(
            ot, prim, x, y, w, h, u, v, clutIndex);
    }
    return GameQueueTexturedRect(
        ot, prim, x, y, w, h, u, v, uSpan, vSpan, clutIndex, tpage);
}
/*
 * DR_MODE, 12 bytes: SetDrawMode(prim, 0, 1, tpage, &g_DrawModeEnv) + AddPrim.
 * This is the "blend packet" the GameDraw* emitters above append when their
 * alpha argument is not 0xFF. SetDrawModePacket only fills the packet in
 * place (no AddPrim, no cursor advance) and has no callers in the retail EXE.
 */

/*
 * A third font, separate from the small (6x12) / large (8x16) pair above:
 * fixed 8x8 SPRT_8 cells on tpage 9, uv from the two-byte-per-glyph table
 * g_Font8x8Cells indexed by (ch - 0x20), 8 pixels of advance per character. Each
 * variant closes the run with its own DR_MODE packet (tpage 9 / 0x29 / 0x49)
 * and stores the render-state cursor back itself.
 */
void DrawText8x8(s32 x, s32 y, const char *str, s32 clutIndex);
void GameDrawText8x8Shaded(s32 x, s32 y, const char *str, s32 clutIndex,
                           u8 intensity);
void DrawText8x8Trans(s32 x, s32 y, const char *text, s32 clutIndex);
/*
 * The proportional font: 12-pixel-tall SPRT cells. Characters below 'a' use
 * the {u, v} pairs at g_PropFontU with fixed 12x12 cells; 'a'..'u' and 'v'+ use
 * the {u, v, width, advance} rows at g_WordFontU / g_HighFontU, so lowercase is
 * proportionally spaced. Space advances 12 and emits nothing.
 *
 * Those rows are four-byte records and read like one, but they stay four
 * parallel u8 arrays indexed at stride 4: folding them into a struct changes
 * what gcc 2.6.3 may alias (MEM_IN_STRUCT_P) and moves the emitted code.
 * `intensity` == 0x100 selects the opaque raw-texture path (SetShadeTex);
 * anything else is written to r = g = b with SetSemiTrans.
 * DrawProportionalText is the wrapper that passes 0x100.
 */
void GameDrawProportionalTextShaded(
    s32 x,
    s32 y,
    const char *str,
    s32 clutIndex,
    s32 intensity);
void DrawProportionalText(s32 x, s32 y, const char *str, s32 clutIndex);

/*
 * Per-object GTE setup: writes (object position - camera position) as an
 * SVECTOR in the shared object workspace, runs it through the render state's
 * view matrix, scales the result by 4 into the translation matrix, then
 * installs the supplied rotation and calculated translation in the GTE.
 */
void SetGteObjectMatrix(LVec *position, Matrix *rotation);

/*
 * The environment colour timeline and the sky it feeds. The state is nine
 * 12-byte { cur, from, to } RGB slots in g_EnvironmentColors - slot 0 the GTE
 * far/fog colour, 1..8 the sky gradient - lerped every frame by UpdateEnvironment.
 */
/* One packed RGB triple of that timeline. The block starts at 0x801E3FB6, i.e.
 * 2 mod 4, so every word in it is half-aligned and must be declared packed --
 * the compiler emits lwl/lwr, not lw. */
/* One timeline slot: the live colour and the pair it is being lerped between.
 * LoadEnvironmentCue rolls `cur` into `from` and the cue's value into `to`;
 * UpdateEnvironment walks `cur` across over g_EnvLerpDuration frames. */
/* The nine slots. [0] is the GTE far/fog colour (SetFarColor takes its three
 * bytes; its unused fourth byte carries auxiliary state), [1..8] the sky-gradient bands.
 * Only six are lerped on a given course: [5]/[6] and [7]/[8] are alternates
 * picked by g_CourseIndex == 2. */
/* Jumps that timeline to `time` and applies one frame, then programs
 * SetFarColor + SetFogNear. */
void SeekEnvironmentScript(s32 targetTime);
/* The backdrop: half a 16-segment panorama cylinder over gradient bands shaded
 * between successive colour slots. */
void DrawSkyBackground(void);

/* Environment mode of the loaded course variant, from variant data +0x2C. Also
 * the index of the target 48-byte (16 x RGB) sky palette in g_EnvPaletteTable;
 * mode 2 alone gets clear fog (SetFogNear ramps to 0x7FFF, else to 0x1770). */
extern s16 g_EnvironmentMode;
/* The mode the previous variant had; the sky-CLUT lerp's source palette. */
extern s32 g_EnvironmentModePrev;
/* Sky palette records, 48 bytes each, indexed by environment mode. Installed
 * from the loaded environment block by InstallTrackRuntimeAssetPack. */
typedef struct EnvironmentPalette {
    Rgb colors[16];
} EnvironmentPalette;

enum { ENVIRONMENT_PALETTE_COUNT = 5 };

extern EnvironmentPalette *g_EnvPaletteTable;
/* The 16 interpolated BGR555 entries uploaded to VRAM at (0xE0, 0x1E6). */
extern u16 g_EnvironmentClut[16];
/* g_EnvironmentMode == 4. Picks DrawStaticScenery's model 0x3B over 0x3A
 * and the `flags & 2` prop set over `flags & 1`; also forwarded to the render state
 * 0x1F800084 by every car/track renderer. */
extern s32 g_IsEnvironmentMode4;
/* That forwarding slot is g_RenderState.envMode4 in game/render_state.h. */

/*
 * Per-view cell culling, rebuilt every frame by BuildVisibleCells and swapped in
 * lockstep for the mirror pass. Per-file types.
 *   g_VisibleCellMask  g_VisibleCellMask  32 words, mask[sy] |= 1 << sx over the grid
 *   g_VisibleCellList  g_VisibleCellList  the matching visible-cell record list
 *   g_SceneLightMatrix g_SceneLightMatrix  assigned from a per-scene constant, then
 *                                  combined per object and set with gte_SetLightMatrix
 */

extern s16 g_CarModelBankTable[][2];
extern Matrix g_MirrorViewMatrix;
extern u8 g_CarModelByCourse[][11];
extern s16 g_MirrorViewEnabled;
extern s32 g_ModelBankCount;
extern s16 g_NegconSteer;
extern s32 g_SetupArrowPulse;

void ApplyZoneLighting(s32 zone, Matrix *mtx);
void EndMirrorPass(void);
void RestoreColorMatrix(void);
s32 rsin(s32 angle);
s32 rcos(s32 angle);
void ApplyMatrixLV(void *matrix, const s32 *input, s32 *output);
void SubmitTerrainCells(void *ctx, void *cells, s32 count);
void SetTrackTexturePageNow(s32 trackSection);

extern Rect g_TrackTextureRowRect;
extern u8 g_CarMirrorBadgeStyles[];
extern u8 g_MirrorBadgeTexU[];
extern u8 g_MirrorBadgeTexV[];
extern u8 g_MirrorBadgeWidths[];
extern GameSpriteDesc g_RaceHudSpriteDescsGp[];
extern GameSpriteDesc g_RaceHudSpriteDescsTimeTrial[];
extern Matrix g_CameraMatrixSaved;
extern s32 g_MenuRowFlashLevels[];
extern s32 g_MenuCursorPulsePhase;
extern Vec4 g_MirrorVisibleCellList[];
extern u32 g_MirrorVisibleCellMask[];
extern u8 g_TrackTextureShadowPage[256];
extern s16 g_AtanTable[];
extern u8 g_Font8x8Cells[];
extern u8 g_HighFontCell[4];
#define g_HighFontU g_HighFontCell
#define g_HighFontV (g_HighFontCell + 1)
#define g_HighFontWidth (g_HighFontCell + 2)
#define g_HighFontYOffset (g_HighFontCell + 3)
extern s32 g_MirrorPanelY;
extern s32 g_MirrorUnlocked;
extern s32 g_NegconPlayScale[];
extern u8 g_PropFontCells[0x80];
#define g_PropFontU g_PropFontCells
#define g_PropFontV (g_PropFontCells + 1)
extern s32 g_TrackTextureCursorRow;
extern s32 g_TrackTexturePageWanted;
extern s32 g_TrackTextureTargetRow;
extern u8 g_WordFontCells[40];
#define g_WordFontU g_WordFontCells
#define g_WordFontV (g_WordFontCells + 1)
#define g_WordFontWidth (g_WordFontCells + 2)
#define g_WordFontAdvance (g_WordFontCells + 3)

void DecDCTReset(s32 mode);
void DrawMinuteSecondTime(s32 x, s32 y, s32 ticks, s32 color);
s32 Gpu_Reset(s32 mode);
void MatrixApplyVectorComponents(Matrix *mtx, s32 x, s32 y, s32 z, s32 *outX,
                                 s32 *outY, s32 *outZ);
void MatrixApplyZRotation(Matrix* mtx, s32 degrees);

#endif
