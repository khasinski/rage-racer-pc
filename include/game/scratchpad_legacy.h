#ifndef GAME_SCRATCHPAD_LEGACY_H
#define GAME_SCRATCHPAD_LEGACY_H

#include "common.h"
#include "game/render_workspace.h"
#include "game/vector.h"
#include "psyq/gte.h"

/* Emulation of the PS1's 0x400-byte scratch RAM for recovered algorithms
 * whose word offsets are still part of their implementation. */
extern u8 g_RageScratchpad[0x400];

#define SCRATCHPAD_ADDR ((void *)g_RageScratchpad)
#define RAGE_SCRATCH_ADDRESS(offset) ((void *)(g_RageScratchpad + (offset)))
#define SCRATCHPAD_AS(type) ((type *)SCRATCHPAD_ADDR)
#define SCRATCHPAD_BYTES SCRATCHPAD_AS(u8)

#define SCRATCH_OBJECT_MATRIX_WORK ((ObjectMatrixWork *)RAGE_SCRATCH_ADDRESS(0x11C))

typedef struct ScratchLegacyViewWords {
    s32 words[10];
} ScratchLegacyViewWords;

typedef union ScratchBlockAddress {
    s32 *words;
    Block16 *blocks;
} ScratchBlockAddress;

static inline void LoadScratchLegacyView(ScratchLegacyViewWords *legacy) {
    legacy->words[2] = RENDER_VIEW_X;
    legacy->words[3] = RENDER_VIEW_Y;
    legacy->words[4] = RENDER_VIEW_Z;
    legacy->words[5] = g_RenderWorkspace.reserved14;
    legacy->words[6] = RENDER_VIEW_ANGLE_X;
    legacy->words[7] = RENDER_VIEW_ANGLE_Y;
    legacy->words[8] = RENDER_VIEW_ANGLE_Z;
    legacy->words[9] = g_RenderWorkspace.depth;
}

static inline void StoreScratchLegacyView(const ScratchLegacyViewWords *legacy) {
    RENDER_VIEW_X = legacy->words[2];
    RENDER_VIEW_Y = legacy->words[3];
    RENDER_VIEW_Z = legacy->words[4];
    g_RenderWorkspace.reserved14 = legacy->words[5];
    RENDER_VIEW_ANGLE_X = legacy->words[6];
    RENDER_VIEW_ANGLE_Y = legacy->words[7];
    RENDER_VIEW_ANGLE_Z = legacy->words[8];
    g_RenderWorkspace.depth = legacy->words[9];
}

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

#define CAR_TRACK_SCRATCH ((CarTrackScratch *)RAGE_SCRATCH_ADDRESS(0x11C))
#define CAR_TRACK_POINT_RADIUS (*(s32 *)RAGE_SCRATCH_ADDRESS(0x130))

#endif
