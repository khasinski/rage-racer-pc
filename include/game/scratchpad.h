#ifndef GAME_SCRATCHPAD_H
#define GAME_SCRATCHPAD_H

#include "common.h"

#include <stddef.h>
#include "game/vector.h"
#include "psyq/gte.h"

/*
 * The working state the renderer and the car code keep between calls.
 *
 * On the PS1 all of this lived in the kilobyte of fast RAM at 0x1F800000, and
 * the game packed unrelated things into it by byte offset, overlaying two
 * different structures on the same address because only one of them was in
 * use at a time. There is no fast RAM here and nothing to pack into, so each
 * piece has its own storage of its own type; what is left is an ordinary
 * struct of named fields.
 */
typedef struct GameScratchpadRenderState {
    void *packetCursor;
    void *primData;
    s32 viewX;
    s32 viewY;
    s32 viewZ;
    s32 reserved14;
    s32 viewAngleX;
    s32 viewAngleY;
    s32 viewAngleZ;
    s32 depth;
    Matrix matrix;
    void *courseBank;
    void *modelModels;
    void *modelTable1;
    void *modelNormals;
    void *cellTable;
    void *cellFaces;
    s32 otShift;
    s32 orderingFlag;
    s32 faceOtShift;
    s32 mode;
    u8 ft4Color[4];
    u8 gt4Color[4];
    s16 x0;
    s16 y0;
    s16 x1;
    s16 y1;
    s32 envMode4;
} GameScratchpadRenderState;

typedef struct ObjectMatrixWork {
    s16 relative[3];
    s16 pad06;
    LVec view;
    s32 pad14;
    Matrix mtx;
} ObjectMatrixWork;

extern GameScratchpadRenderState g_RageScratchpadState;

/*
 * The matrix the model path builds a transform in. It shared an address with
 * the car's track working set, which is safe only for as long as no frame
 * wants both; it does not share one now.
 */
extern ObjectMatrixWork g_ObjectMatrixWork;
#define SCRATCH_OBJECT_MATRIX_WORK (&g_ObjectMatrixWork)

typedef union ScratchViewCoordinate {
    s32 value;
    struct {
        u16 low;
        u16 high;
    } half;
} ScratchViewCoordinate;

typedef struct ScratchViewCoordinates {
    ScratchViewCoordinate x;
    ScratchViewCoordinate y;
    ScratchViewCoordinate z;
} ScratchViewCoordinates;

typedef union ScratchViewPosition {
    ScratchViewCoordinates components;
    LVec vector;
} ScratchViewPosition;

typedef struct ScratchViewState {
    ScratchViewPosition position;
    s32 reserved14;
    s32 angleX;
    s32 angleY;
    s32 angleZ;
} ScratchViewState;

/* Local compatibility view for algorithms recovered with the original PS1
 * word indices.  Keep it off the canonical scratch state: words 0 and 1 were
 * 32-bit pointers on PS1 and cannot share storage with native pointers. */
typedef struct ScratchLegacyViewWords {
    s32 words[10];
} ScratchLegacyViewWords;

typedef union ScratchBlockAddress {
    s32 *words;
    Block16 *blocks;
} ScratchBlockAddress;

typedef union CarTrackRadius {
    s32 value;
    struct {
        u16 low;
        u16 high;
    } half;
} CarTrackRadius;

typedef struct CarTrackScratch {
    s32 arcCenterX;
    s32 arcCenterZ;
    s32 carToCenterX;
    s32 carToCenterZ;
    CarTrackRadius carRadius;
    CarTrackRadius pointRadius;
    CarTrackRadius nextPointRadius;
    u8 pad1C[8];
    s32 pointToCenterX;
    s32 nextPointToCenterX;
    s32 pointToCenterZ;
    s32 nextPointToCenterZ;
    s32 headingSin;
    s32 headingCos;
    s32 knockbackMode;
    u8 pad40[0x20];
    u16 offsetX;
    s16 offsetY;
    s16 offsetZ;
    s16 offsetPad;
    s32 correctionX;
    s32 correctionY;
    s32 correctionZ;
    s32 reserved74;
    s16 curveMode;
    s16 arcIndex;
    s16 arcSpan;
    s16 sweptAngle;
    s16 pointAngle;
    s16 nextPointAngle;
    s16 arcLateral;
    s16 trackWidth;
    s16 rightHalfWidth;
    s16 leftHalfWidth;
    s16 relativeHeading;
    s16 crossSlope;
    s16 heading;
    s16 surfacePitch;
    s16 camberAngle;
    u16 segmentLength;
} CarTrackScratch;

/* Where the car code works out where it sits on the track. */
extern CarTrackScratch g_CarTrackScratch;

/*
 * The primitive-packing cursor. Every emitter packs a GPU packet at it, bumps
 * it past the packet and stores it back, so each one spells the slot with the
 * packet type it is building. SCRATCH_PRIM_CURSOR_AS gives that type without
 * repeating the address; The retail code also carried the cursor
 * as an integer in this slot; reading it back that way would take half of a
 * pointer here, so that spelling is gone.
 */
#define SCRATCH_PRIM_CURSOR_AS(type) (*(type **)&g_RageScratchpadState.packetCursor)
#define SCRATCH_PRIM_CURSOR          SCRATCH_PRIM_CURSOR_AS(void)
#define SCRATCH_PRIM_CURSOR_VOLATILE (*(u8 *volatile *)&g_RageScratchpadState.packetCursor)
#define SCRATCH_PRIM_CURSOR_SLOT     (&SCRATCH_PRIM_CURSOR_VOLATILE)

/* Ordering table the emitters link finished packets into. */
#define SCRATCH_OT_BASE_AS(type)     (*(type **)&g_RageScratchpadState.primData)
#define SCRATCH_OT_BASE              SCRATCH_OT_BASE_AS(void)

/* View transform consumed by the model render path. SetCameraRotMatrix builds
 * the matrix at 0x28 from the three angles; the position words are the camera
 * translation passed to SetGteObjectMatrix. */
/*
 * The camera words are also read as one block, so the two spellings have to
 * agree on where each word sits. They are checked rather than trusted.
 */
#define SCRATCH_VIEW_STATE   ((ScratchViewState *)&g_RageScratchpadState.viewX)
_Static_assert(sizeof(ScratchViewState) ==
                   offsetof(GameScratchpadRenderState, depth) -
                       offsetof(GameScratchpadRenderState, viewX),
               "the camera block and the camera fields have drifted apart");
_Static_assert(offsetof(ScratchViewState, angleX) ==
                   offsetof(GameScratchpadRenderState, viewAngleX) -
                       offsetof(GameScratchpadRenderState, viewX),
               "the camera block puts the angles somewhere else");

static inline void LoadScratchLegacyView(ScratchLegacyViewWords *legacy) {
    legacy->words[2] = g_RageScratchpadState.viewX;
    legacy->words[3] = g_RageScratchpadState.viewY;
    legacy->words[4] = g_RageScratchpadState.viewZ;
    legacy->words[5] = g_RageScratchpadState.reserved14;
    legacy->words[6] = g_RageScratchpadState.viewAngleX;
    legacy->words[7] = g_RageScratchpadState.viewAngleY;
    legacy->words[8] = g_RageScratchpadState.viewAngleZ;
    legacy->words[9] = g_RageScratchpadState.depth;
}

static inline void StoreScratchLegacyView(const ScratchLegacyViewWords *legacy) {
    g_RageScratchpadState.viewX = legacy->words[2];
    g_RageScratchpadState.viewY = legacy->words[3];
    g_RageScratchpadState.viewZ = legacy->words[4];
    g_RageScratchpadState.reserved14 = legacy->words[5];
    g_RageScratchpadState.viewAngleX = legacy->words[6];
    g_RageScratchpadState.viewAngleY = legacy->words[7];
    g_RageScratchpadState.viewAngleZ = legacy->words[8];
    g_RageScratchpadState.depth = legacy->words[9];
}

/* Course object bank. SubmitCourseModel / SubmitCourseModel2 (0x800296BC,
 * 0x80029E58) load it and index by model id; size is g_CourseModelCount. */

/* Model bank cursor, pointed at one g_ModelBanks entry by SelectModelBank.
 * MODELS is the model pointer array (bank + 0xC) that SubmitModel indexes by
 * id << 2 (0x80028DEC); NORMALS is bank[2] rebased, the 8-byte SVECTORs the
 * Emit*G4 / Emit*GT4 quad builders index by id << 3 and feed to ncct/nccs
 * (0x80029168). TABLE1 is bank[1] rebased; nothing in the disassembled engine
 * reads it, so it is named for where it comes from, not what it holds. */

/* Terrain: the per-cell record array SubmitTerrainCells indexes by cell id
 * (0x80028078) and the face array SubmitTerrainCellFaces walks (0x80028168). */

/* The srav amount that turns a transformed Z into an ordering-table index:
 * OT_SHIFT on the cell-face path (0x800283C0), FACE_OT_SHIFT on the mode-1
 * path, where it is read as a halfword (0x80028474). InitRenderState sets
 * OT_SHIFT from its parameter, 5 for the race scene and 1 for two menus. */

/* Mirror flag. Non-zero makes the engine negate the GTE rotation matrix
 * (0x80028000, 0x80028E00); track/draw_terrain_cells.c compares it against
 * g_MirrorMode. Same word as the struct's `orderingFlag`. */

/* Two packed GTE RGBC words, read whole with lwc2 into cop2 register 6:
 * EmitPolyFT4Fog takes 0x70 (0x80029468), EmitPolyGT4Fog takes 0x74
 * (0x80029620). The fourth byte is the GPU primitive code the emitter stamps
 * into the packet, 0x2C for a 40-byte POLY_FT4 and 0x3C for a 52-byte
 * POLY_GT4. */

/* Screen clip rectangle every emitter rejects primitives against; the same
 * four halfwords as the struct's x0/y0/x1/y1. menu/frontend.c raises Y1 to 0x1E0 for
 * the 480-line modes. */
#define SCRATCH_CLIP_X0        (*(u16 *)&g_RageScratchpadState.x0)
#define SCRATCH_CLIP_Y0        (*(u16 *)&g_RageScratchpadState.y0)
#define SCRATCH_CLIP_X1        (*(u16 *)&g_RageScratchpadState.x1)
#define SCRATCH_CLIP_Y1        (*(u16 *)&g_RageScratchpadState.y1)

/* g_IsEnvironmentMode4, forwarded here by every car and track renderer for the
 * GTE engine to read. Spelled as a macro rather than an `extern ... asm()`
 * symbol on purpose: the extern form lets gcc 2.6.3 hold the address in a
 * register across calls, which changes the output. */

#endif
