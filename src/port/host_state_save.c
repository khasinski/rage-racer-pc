/*
 * Retail state belonging to the memory card: the card and file inventory, the
 * save and load progress, the block the game writes, and the settings that
 * are only interesting because they survive a power cycle.
 *
 * A saved setting is read by whatever screen changes it as well as by the
 * card code, so the ones here are the ones the card code reads most. Order is
 * retail's address order.
 */

#include <stddef.h>

#include "common.h"

unsigned char g_FmtString[8] __attribute__((aligned(16))) = "%s";
unsigned char g_MsgSaveChecksumOk[8] __attribute__((aligned(16))) = "ok \?";
unsigned char g_FmtSaveChecksum[20] __attribute__((aligned(16))) = {0x64,0x20,0x25,0x78,0x6c,0x20,0x2c,0x20,0x73,0x75,0x6d,0x20,0x25,0x78,0x6c,0x0a,0x00,0x00,0x00,0x00};
unsigned char g_SaveDefaults[104] __attribute__((aligned(16))) = {0x00,0x02,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x00,0x01,0x01,0x00,0x00,0x00,0x00,0x04,0x01,0x02,0x02,0x00,0x00,0x00,0x00,0x01,0x00,0x03,0x03,0x01,0x00,0x00,0x00,0x00,0x00,0x04,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x05,0x05,0x00,0x00,0x00,0x00,0x00,0x01,0x06,0x06,0x00,0x00,0x00,0x00,0x02,0x00,0x07,0x07,0x00,0x00,0x00,0x00,0x02,0x01,0x08,0x08,0x00,0x00,0x00,0x00,0x03,0x01,0x09,0x09,0x00,0x00,0x00,0x00,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x01,0x00,0x00,0x00,0x00,0x00};
s32 g_McLastCardStatus;
s32 g_McMenuPage;
s32 g_McMenuRowCursor;
unsigned char g_McSlotCursor[36] __attribute__((aligned(16))) =
    "\0\0\0\0"
    "SAVE\0\0\0\0\0\0"
    "LOAD\0\0\0\0\0\0"
    "EXIT";
s32 g_McActionState;
s32 g_McActionResult;
s32 g_McConfirmChoice;
s32 g_McActionTimer;
s32 g_McActionBusy;
s32 g_McErrorCountdown = 3;
s32 g_McErrorPending;
s32 g_McLastSlot;
s32 g_McHwEventIoe;
s32 g_McHwEventError;
s32 g_McHwEventTimeout;
s32 g_McHwEventNew;
s32 g_McSwEventIoe;
s32 g_McSwEventError;
s32 g_McSwEventTimeout;
s32 g_McSwEventNew;
unsigned char g_SaveIconRect[8] __attribute__((aligned(16)));
s32 g_McSlotUsedMask;
unsigned char g_McSaveHeaders[384] __attribute__((aligned(16)));
s32 g_McNoCardTicks;
s32 g_McErrorTicks;
s32 g_McLastMenuState;
s32 g_McSettleTicks;
s32 g_McCardOkFrames;
unsigned char g_McActionElapsed[20] __attribute__((aligned(16)));
s32 g_McMenuState;
s32 g_McCardStatus;
s32 g_McMenuSelection;
unsigned char g_McMenuPhase[8] __attribute__((aligned(16)));
s32 g_McFromLoadMenu;
s32 g_McSaveMode;
s32 g_McCardFileCount;
s32 g_McFreeBlocks;
unsigned char GameMenuLoadPhase[8] __attribute__((aligned(16)));
s32 g_McMenuRowCount;
unsigned char g_McDirEntries[600] __attribute__((aligned(16)));
s32 g_McFadeStep;
s32 g_McFadeLevel;
unsigned char g_McStatusState[8] __attribute__((aligned(16)));
unsigned char g_ExtraGrandPrixCourseProgress[8] __attribute__((aligned(16)));
s32 g_BgmVolumeSetting;
s32 g_McPollTicks;
s32 g_McStatusResult;
unsigned char g_ExtraGrandPrixCars[104] __attribute__((aligned(16)));
unsigned char g_GrandPrixCourseProgress[8] __attribute__((aligned(16)));
unsigned char g_TimeAttackCars[104] __attribute__((aligned(16)));
unsigned char g_GrandPrixCars[104] __attribute__((aligned(16)));
unsigned char g_MaxClassReached[8] __attribute__((aligned(16)));
s32 g_SaveElapsedTicks;
s32 g_McPollStatus;
s32 g_SfxVolumeSetting;
