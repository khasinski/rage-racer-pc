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
#include "game/car.h"
#include "game/memcard_types.h"
#include "game/save_format.h"
#include "game/save_types.h"
#include "psyq/gpu_types.h"
#include "psyq/kernel.h"

char g_FmtString[8] __attribute__((aligned(16))) = "%s";
char g_MsgSaveChecksumOk[8] __attribute__((aligned(16))) = "ok \?";
char g_FmtSaveChecksum[20] __attribute__((aligned(16))) = {0x64,0x20,0x25,0x78,0x6c,0x20,0x2c,0x20,0x73,0x75,0x6d,0x20,0x25,0x78,0x6c,0x0a,0x00,0x00,0x00,0x00};
CarEntry g_SaveDefaults[GAME_CAR_COUNT] = {
    {0x00, 0x02, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}},
    {0x00, 0x03, 0x00, 0x01, 0x01, 0x00, {0x00, 0x00}},
    {0x00, 0x04, 0x01, 0x02, 0x02, 0x00, {0x00, 0x00}},
    {0x00, 0x01, 0x00, 0x03, 0x03, 0x01, {0x00, 0x00}},
    {0x00, 0x00, 0x00, 0x04, 0x04, 0x00, {0x00, 0x00}},
    {0x00, 0x00, 0x00, 0x05, 0x05, 0x00, {0x00, 0x00}},
    {0x00, 0x00, 0x01, 0x06, 0x06, 0x00, {0x00, 0x00}},
    {0x00, 0x02, 0x00, 0x07, 0x07, 0x00, {0x00, 0x00}},
    {0x00, 0x02, 0x01, 0x08, 0x08, 0x00, {0x00, 0x00}},
    {0x00, 0x03, 0x01, 0x09, 0x09, 0x00, {0x00, 0x00}},
    {0x00, 0x04, 0x00, 0x00, 0x00, 0x00, {0x00, 0x00}},
    {0x00, 0x00, 0x01, 0x00, 0x00, 0x00, {0x00, 0x00}},
    {0x00, 0x03, 0x01, 0x00, 0x00, 0x00, {0x00, 0x00}},
};
s32 g_McLastCardStatus;
s32 g_McMenuPage;
s32 g_McMenuRowCursor;
s32 g_McSlotCursor;
char g_McModeLabels[32] __attribute__((aligned(16))) =
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
s32 g_McSlotUsedMask;
GameSaveHeaderRow g_McSaveHeaders[MEMORY_CARD_SAVE_SLOT_COUNT];
s32 g_McNoCardTicks;
s32 g_McErrorTicks;
s32 g_McLastMenuState;
s32 g_McSettleTicks;
s32 g_McCardOkFrames;
s32 g_McActionElapsed;
s32 g_McMenuState;
s32 g_McCardStatus;
s32 g_McMenuSelection;
s32 g_McMenuPhase;
s32 g_McFromLoadMenu;
s32 g_McSaveMode;
s32 g_McCardFileCount;
s32 g_McFreeBlocks;
s32 GameMenuLoadPhase;
s32 g_McMenuRowCount;
DirEntry g_McDirEntries[MEMORY_CARD_MAX_FILES];
s32 g_McFadeStep;
s32 g_McFadeLevel;
s32 g_McStatusState;
CourseProgressState g_ExtraGrandPrixCourseProgress;
s32 g_BgmVolumeSetting;
s32 g_McPollTicks;
s32 g_McStatusResult;
CarEntry g_ExtraGrandPrixCars[GAME_CAR_COUNT];
CourseProgressState g_GrandPrixCourseProgress;
CarEntry g_TimeAttackCars[GAME_CAR_COUNT];
CarEntry g_GrandPrixCars[GAME_CAR_COUNT];
s32 g_MaxClassReached[2];
s32 g_SaveElapsedTicks;
s32 g_McPollStatus;
s32 g_SfxVolumeSetting;
