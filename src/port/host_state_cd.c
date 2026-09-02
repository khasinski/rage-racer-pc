/*
 * Retail state for the CD drive and its audio: the command in flight and how
 * far it has got, the mixer's four channel volumes and their presets, the
 * track being played and the fade over it.
 *
 * The disc is its own subsystem rather than part of audio: this is the drive
 * being commanded, not sound being made. Order is retail's address order.
 */

#include <stddef.h>

#include "common.h"
#include "psyq/cd_location.h"
#include "psyq/cd_types.h"

unsigned char g_CdMixPresets[8] __attribute__((aligned(16))) = {0x7f,0x00,0x7f,0x00,0x3f,0x3f,0x3f,0x3f};
s32 g_CdRestartOnResume;
s32 g_CdMixPreset;
s32 g_CdTrackPending = -1;
s32 g_CdCommandPending = -1;
s32 g_CdTrackStep;
s32 g_CdCommandStep;
CdlLOC g_CdTrackElapsedLoc;
u8 g_CdModeParam;
unsigned char g_CdLocResult[8] __attribute__((aligned(16)));
u32 g_CdMixLL;
u32 g_CdMixLR;
u32 g_CdMixRR;
u32 g_CdMixRL;
u32 g_CdMixFullLL;
u32 g_CdMixFullLR;
u32 g_CdMixFullRR;
u32 g_CdMixFullRL;
u8 g_CdVolume;
CdlFILE g_CdSearchFile;
u8 g_CdCurrentTrack;
s32 g_CdFadeFrames;
