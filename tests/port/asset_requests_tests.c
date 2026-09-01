#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/team_logo.h"

#include <stdio.h>
#include <string.h>

AssetRequestType g_AssetRequestType;
s32 g_AssetLoadState;
s32 g_LoadBuffer[4];
u8 *g_AssetBlockPtr;
u8 *g_AssetBlockPtr2;
u8 *g_ImageBlockBuffer;
u8 *g_AssetBase;
u8 *g_AssetLoadCursor;
u8 *g_AssetSubBlockPtr;
u16 g_TeamLogoClut[16];
TeamLogoCanvas g_TeamLogoCanvas;
TeamLogoRect g_TeamLogoRect;
Rect g_TeamLogoClutRect;

static s32 s_loadResult;
static s32 s_resetCalls;
static s32 s_closeCalls;
static s32 s_failures;

s32 LoadAsset(s32 slot, void *destination) {
    (void)slot;
    (void)destination;
    return s_loadResult;
}
void UploadLoadBufferImage(void) {}
void StartAudioSlotLoad(s32 slot, void *header, void *body, s32 sequence) {
    (void)slot; (void)header; (void)body; (void)sequence;
}
s32 PollAudioSlotLoad(void) { return 0; }
void InstallResourceData(void *data) { (void)data; }
void UploadImageAsset(void *data) { (void)data; }
void CloseLoadedAudioSlots(void) { s_closeCalls++; }
void ResetCdAudioState(void) { s_resetCalls++; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

int main(void) {
    union {
        max_align_t alignment;
        u8 bytes[128];
    } pack;
    GameSceneAssetHeader *header = (GameSceneAssetHeader *)pack.bytes;

    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(RequestSelectBgmAssets() == 1, "new request remains pending");
    Check(g_AssetRequestType == ASSET_REQUEST_SELECT_BGM,
          "new request type");
    Check(g_AssetLoadState == 1, "new request first load state");
    Check(s_resetCalls == 1, "new request resets CD audio");
    Check(RequestSelectBgmAssets() == 1, "busy request remains pending");

    g_AssetLoadState = 0;
    Check(RequestSelectBgmAssets() == 0, "matching request acknowledges");
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE,
          "acknowledged request becomes idle");

    memset(&pack, 0, sizeof(pack));
    header->offsets[0] = 16;
    header->offsets[1] = 32;
    header->offsets[2] = 64;
    g_AssetBase = pack.bytes;
    g_AssetLoadState = 1;
    s_loadResult = 1;
    LoadSelectBgmAssets();
    Check(s_closeCalls == 1, "BGM load closes previous audio slots");
    Check(g_AssetLoadState == 0, "BGM load completes");
    Check(g_AssetBlockPtr == pack.bytes + 16, "BGM first block");
    Check(g_AssetBlockPtr2 == pack.bytes + 32, "BGM second block");
    Check(g_AssetSubBlockPtr == pack.bytes + 64, "BGM third block");

    g_AssetLoadState = 2;
    s_loadResult = 0;
    g_AssetBlockPtr = NULL;
    LoadSelectBgmAssets();
    Check(g_AssetLoadState == 2, "incomplete BGM load stays pending");
    Check(g_AssetBlockPtr == NULL, "incomplete BGM load installs nothing");

    if (s_failures != 0) return 1;
    puts("asset requests acknowledge and install their BGM blocks");
    return 0;
}
