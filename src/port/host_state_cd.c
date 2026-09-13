/*
 * Retail state for the CD drive and its audio: the command in flight and how
 * far it has got, the mixer's four channel volumes and their presets, the
 * track being played and the fade over it.
 *
 * The disc is its own subsystem rather than part of audio: this is the drive
 * being commanded, not sound being made. Order is retail's address order.
 */

#include "game/cd.h"

unsigned char g_CdMixPresets[8] = {0x7f,0x00,0x7f,0x00,0x3f,0x3f,0x3f,0x3f};
Cd g_Cd = {
    .pendingTrack = -1,
    .pendingCommand = CD_COMMAND_NONE,
};
