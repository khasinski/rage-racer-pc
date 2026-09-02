/*
 * Retail state for getting data off the disc and into memory: the load
 * buffer, the block and cursor the loader advances through it, the request
 * queue, and the streaming position.
 *
 * g_LoadBuffer is a megabyte, and the bounds helper that goes with it is here
 * rather than in a header because sizeof needs the definition. Order is
 * retail's address order.
 */

#include <stddef.h>
#include <stdint.h>

#include "common.h"
#include "game/car.h"
#include "psyq/gpu.h"

typedef struct CarImageData CarImageData;
typedef struct GameCdLoadEntry GameCdLoadEntry;

char g_MsgResOk[8] __attribute__((aligned(16))) = "res ok!";
char g_MsgEventOk[12] __attribute__((aligned(16))) = "event ok\n";
s32 g_AssetLoadState;
u8 g_CarModelBaseIndex[GAME_CAR_COUNT] __attribute__((aligned(16))) = {
    0, 4, 7, 9, 14, 18, 21, 23, 26, 28, 29, 30, 31
};
u8 g_CarModelUnlockBase[GAME_CAR_COUNT] __attribute__((aligned(16))) = {
    1, 2, 3, 0, 1, 2, 3, 2, 3, 4, 5, 5, 5
};
s32 g_AssetRequestType;
Rect g_TrackTextureRect __attribute__((aligned(16))) = {576, 256, 448, 256};
Rect g_TeamLogoClutLoadRect __attribute__((aligned(16))) = {80, 485, 16, 1};
GpuRectPacked g_TeamLogoClutMoveRect __attribute__((aligned(16))) = {
    0x00EC03F0, 16, 1
};
s32 g_PendingCarModelIndex;
u32 g_CarModelSlot;
s32 g_LoadBuffer[1037896 / sizeof(s32)] __attribute__((aligned(16)));
/* Extent of the boot load buffer, for the override bounds check. */
unsigned long PortLoadBufferRoomAt(const void *at) {
    const uintptr_t address = (uintptr_t)at;
    const uintptr_t begin = (uintptr_t)g_LoadBuffer;
    const uintptr_t end = begin + sizeof(g_LoadBuffer);

    if (address >= begin && address < end)
        return (unsigned long)(end - address);
    return 0;
}
u32 g_StreamSectorLimit;
u8 *g_AssetBlockPtr2;
u32 g_StreamSectorCount;
u8 *g_AssetLoadCursor;
u8 *g_CarModelBuffer;
CarImageData *g_CarImageSlots[2];
s32 (*g_TrackTextureShadow)[0xE0];
u8 *g_ImageBlockBuffer;
s32 g_SharedAssetWord0;
GameCdLoadEntry *g_StreamLoc;
u8 *g_AssetSubBlockPtr;
u8 *g_AssetBlockPtr;
