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

typedef struct CarImageData CarImageData;
typedef struct GameCdLoadEntry GameCdLoadEntry;

char g_MsgResOk[8] __attribute__((aligned(16))) = "res ok!";
char g_MsgEventOk[12] __attribute__((aligned(16))) = "event ok\n";
s32 g_AssetLoadState;
unsigned char g_CarModelBaseIndex[16] __attribute__((aligned(16))) = {0x00,0x04,0x07,0x09,0x0e,0x12,0x15,0x17,0x1a,0x1c,0x1d,0x1e,0x1f,0x00,0x00,0x00};
unsigned char g_CarModelUnlockBase[16] __attribute__((aligned(16))) = {0x01,0x02,0x03,0x00,0x01,0x02,0x03,0x02,0x03,0x04,0x05,0x05,0x05,0x00,0x00,0x00};
s32 g_AssetRequestType;
unsigned char g_TrackTextureRect[8] __attribute__((aligned(16))) = {0x40,0x02,0x00,0x01,0xc0,0x01,0x00,0x01};
unsigned char g_TeamLogoClutLoadRect[8] __attribute__((aligned(16))) = {0x50,0x00,0xe5,0x01,0x10,0x00,0x01,0x00};
unsigned char g_TeamLogoClutMoveRect[8] __attribute__((aligned(16))) = {0xf0,0x03,0xec,0x00,0x10,0x00,0x01,0x00};
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
