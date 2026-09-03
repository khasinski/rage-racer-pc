/*
 * Retail global state that belongs to no one subsystem.
 *
 * This file was once the whole retail data segment transcribed into C, in the
 * order the addresses happened to fall. It has been split up: each subsystem's
 * state now sits in host_state_<area>.c beside the code that reads it, and
 * obsolete state with no readers has been removed.
 *
 * What is left is the handful the split could not place. The frame counter and
 * the game clock, the scene the port is showing and the timer it has been
 * showing it for, the random seed, the mirror's mode and its view, the FMV
 * state. Every one of them is read from four or five subsystems at once, or
 * only by the port's own scaffolding, which is what makes them nobody's.
 *
 * Values are checked into the port; no runtime PS-EXE loading.
 */

#include <stddef.h>

#include "common.h"
#include "game/render_types.h"
#include "game/vector.h"
#include "psyq/gte.h"

s16 g_CdLoadPhase;
s32 g_FmvStreamEnded;
u32 g_RandomSeed;
s32 g_GameClock;
s32 g_FrameCounter;
VisibleTerrainCell g_MirrorVisibleCellList[64];
s32 g_FmvState;
s32 g_FrontendState;
s32 g_StreamReturnScene;
u32 g_MirrorVisibleCellMask[32];
s32 g_MirrorMode;
Matrix g_MirrorViewMatrix;
s32 g_SceneTimer;
s32 g_SceneId;
s32 g_SkyRowBase;
s32 g_TrackTexturePageWanted;
