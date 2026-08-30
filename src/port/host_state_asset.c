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

#include "common.h"

unsigned char g_MsgResOk[8] __attribute__((aligned(16))) = "res ok!";
unsigned char g_MsgEventOk[12] __attribute__((aligned(16))) = {0x65,0x76,0x65,0x6e,0x74,0x20,0x6f,0x6b,0x0a,0x00,0x00,0x00};
s32 g_AssetLoadState;
unsigned char g_CarModelBaseIndex[16] __attribute__((aligned(16))) = {0x00,0x04,0x07,0x09,0x0e,0x12,0x15,0x17,0x1a,0x1c,0x1d,0x1e,0x1f,0x00,0x00,0x00};
unsigned char g_CarModelUnlockBase[16] __attribute__((aligned(16))) = {0x01,0x02,0x03,0x00,0x01,0x02,0x03,0x02,0x03,0x04,0x05,0x05,0x05,0x00,0x00,0x00};
unsigned char g_AssetRequestType[8] __attribute__((aligned(16))) = {0x00,0x00,0x00,0x00,0};
unsigned char g_TrackTextureRect[8] __attribute__((aligned(16))) = {0x40,0x02,0x00,0x01,0xc0,0x01,0x00,0x01};
unsigned char g_TeamLogoClutLoadRect[8] __attribute__((aligned(16))) = {0x50,0x00,0xe5,0x01,0x10,0x00,0x01,0x00};
unsigned char g_TeamLogoClutMoveRect[8] __attribute__((aligned(16))) = {0xf0,0x03,0xec,0x00,0x10,0x00,0x01,0x00};
s32 g_PendingCarModelIndex;
u32 g_CarModelSlot;
unsigned char g_LoadBuffer[1037896] __attribute__((aligned(16)));
/* Extent of the boot load buffer, for the override bounds check. */
unsigned long PortLoadBufferRoomAt(const void *at) {
    const unsigned char *p = at;
    if (p >= g_LoadBuffer && p < g_LoadBuffer + sizeof(g_LoadBuffer))
        return (unsigned long)(g_LoadBuffer + sizeof(g_LoadBuffer) - p);
    return 0;
}
u32 g_StreamSectorLimit;
unsigned char g_AssetBlockPtr2[8] __attribute__((aligned(16)));
u32 g_StreamSectorCount;
unsigned char g_AssetLoadCursor[8] __attribute__((aligned(16)));
unsigned char g_CarModelBuffer[8] __attribute__((aligned(16)));
unsigned char g_CarImageSlots[56] __attribute__((aligned(16)));
unsigned char g_TrackTextureShadow[8] __attribute__((aligned(16)));
unsigned char g_ImageBlockBuffer[8] __attribute__((aligned(16)));
s32 g_SharedAssetWord0;
unsigned char g_StreamLoc[8] __attribute__((aligned(16)));
unsigned char g_AssetSubBlockPtr[8] __attribute__((aligned(16)));
unsigned char g_AssetBlockPtr[8] __attribute__((aligned(16)));
