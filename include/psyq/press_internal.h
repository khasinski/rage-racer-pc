#ifndef RAGE_PC_PSYQ_PRESS_INTERNAL_H
#define RAGE_PC_PSYQ_PRESS_INTERNAL_H

#include "common.h"
#include "psyq/cd_types.h"

extern volatile u32 *g_InterruptMask;
extern volatile u32 *g_InterruptStatus;

extern volatile u_char *g_StreamCdReg0;
extern volatile u_char *g_StreamCdReg3;
extern volatile StStrHeader *g_StActiveHeader;
extern long g_StBackFrame;
extern u_char g_StBackLoc[];
extern volatile u8 *g_StCdReg0;
extern volatile u8 *g_StCdReg2;
extern volatile u8 *g_StCdReg3;
extern s32 g_StColorMode;
extern s32 g_StCurrentChannel;
extern s32 g_StCurrentFrameCount;
extern s16 g_StCurrentSector;
extern s32 g_StDmaBusy;
extern StCallback g_StEndCallback;
extern s32 g_StEndFrame;
extern StCallback g_StFrameCallback;
extern s32 g_StInterruptPending;
extern s32 g_StInterruptState;
extern s32 g_StNextChannel;
extern s32 g_StNotStream2Mode;
extern s32 g_StReadCursor;
extern s32 g_StRingSize;
extern s32 g_StRingSlot;
extern s32 g_StStartFrame;
extern s32 g_StStreamFlag;
extern s32 g_StWriteCursor;

#endif
