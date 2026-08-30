/*
 * Retail global state that belongs to no one subsystem.
 *
 * This file was once the whole retail data segment transcribed into C, in the
 * order the addresses happened to fall. It has been split up: each subsystem's
 * state now sits in host_state_<area>.c beside the code that reads it, and
 * host_state_unread.c holds what nothing in this port reads at all.
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

s16 g_CdLoadPhase;
s16 g_SpinningSceneryAngle[4] __attribute__((aligned(16))) = {
    0, 64, 128, 256
};
s32 g_FmvStreamEnded;
u32 g_RandomSeed;
s32 g_GameClock;
s32 g_FrameCounter;
unsigned char g_MirrorVisibleCellList[1024] __attribute__((aligned(16)));
unsigned char g_FmvState[8] __attribute__((aligned(16)));
unsigned char g_FrontendState[8] __attribute__((aligned(16)));
s32 g_StreamReturnScene;
unsigned char g_MirrorVisibleCellMask[128] __attribute__((aligned(16)));
s32 g_MirrorMode;
unsigned char g_MirrorViewMatrix[32] __attribute__((aligned(16)));
s32 g_SceneTimer;
s32 g_SceneId;
s32 g_SkyRowBase;
s32 g_TrackTexturePageWanted;
