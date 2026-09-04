/*
 * Retail global state that belongs to no one subsystem.
 *
 * This file was once the whole retail data segment transcribed into C, in the
 * order the addresses happened to fall. It has been split up: each subsystem's
 * state now sits in host_state_<area>.c beside the code that reads it, and
 * obsolete state with no readers has been removed.
 *
 * What is left is the handful the split cannot place: the process clocks, the
 * active scene and its timer, the random seed, and the global mirror mode.
 * Each is read across several otherwise independent subsystems.
 *
 * Values are checked into the port; no runtime PS-EXE loading.
 */

#include "common.h"

u32 g_RandomSeed;
s32 g_GameClock;
s32 g_FrameCounter;
s32 g_MirrorMode;
s32 g_SceneTimer;
s32 g_SceneId;
