#include "common.h"
#include "game/asset.h"
#include "game/menu.h"
#include "game/menu_internal.h"
#include "game/random.h"
#include "game/race.h"
#include "game/render.h"
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
s32 g_SharedAssetWord0;
s16 g_GrandPrixMode;
s16 g_GrandPrixSeries;
s32 g_GrandPrixClass;
s32 g_CourseIndex;
s32 g_MaxClassReached[2];
u16 g_TeamLogoClut[16];
TeamLogoCanvas g_TeamLogoCanvas;
TeamLogoRect g_TeamLogoRect;
Rect g_TeamLogoClutRect;

static s32 s_loadResult;
static s32 s_resetCalls;
static s32 s_closeCalls;
static s32 s_failures;
static s32 s_lastAssetId;
static s32 s_modelBankRegistrations;
static s32 s_randomValue;
static s32 s_selectModelBankCalls;

s32 LoadAsset(s32 slot, void *destination) {
    (void)destination;
    s_lastAssetId = slot;
    return s_loadResult;
}
void UploadLoadBufferImage(void) {}
void StartAudioSlotLoad(s32 slot, void *header, void *body, s32 sequence) {
    (void)slot; (void)header; (void)body; (void)sequence;
}
s32 PollAudioSlotLoad(void) { return 0; }
void InstallResourceData(void *data) { (void)data; }
void UploadImageAsset(GameImageAssetHeaderWord *data) { (void)data; }
void CloseLoadedAudioSlots(void) { s_closeCalls++; }
void ResetCdAudioState(void) { s_resetCalls++; }
void ResetAssetLoader(void) {
    s_resetCalls++;
    g_AssetLoadState = 0;
}
void RegisterModelBank(ModelBankHeader *base, s32 index) {
    (void)base;
    (void)index;
    s_modelBankRegistrations++;
}
void SelectModelBank(s32 index) {
    (void)index;
    s_selectModelBankCalls++;
}
s32 Random15(void) { return s_randomValue; }

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

    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    s_resetCalls = 0;
    RequestOptionScreenAssets();
    Check(g_AssetRequestType == ASSET_REQUEST_OPTION_SCREEN,
          "OPTION request type");
    Check(g_AssetLoadState == 1 && s_resetCalls == 1,
          "OPTION request starts loader and resets CD");
    g_AssetLoadState = 0;
    RequestOptionScreenAssets();
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE,
          "OPTION request acknowledged");

    memset(&pack, 0, sizeof(pack));
    ((OptionScreenAsset *)pack.bytes)->imageOffset = 48;
    g_AssetBase = pack.bytes;
    g_AssetLoadState = 1;
    s_loadResult = 1;
    s_modelBankRegistrations = 0;
    s_selectModelBankCalls = 0;
    LoadOptionScreenAssets();
    Check(s_lastAssetId == 9, "OPTION asset id");
    Check(g_ImageBlockBuffer == pack.bytes + 48, "OPTION image block");
    Check(g_AssetLoadState == 0 && s_modelBankRegistrations == 1 &&
              s_selectModelBankCalls == 1,
          "OPTION asset installed");

    g_GrandPrixMode = 0;
    g_GrandPrixSeries = 0;
    g_MaxClassReached[0] = 4;
    g_CourseIndex = 1;
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    s_randomValue = 3;
    Check(RequestRoundAssets() == 1, "new round request pending");
    Check(g_GrandPrixClass == 3, "time-attack class randomized");
    Check(g_AssetRequestType == ASSET_REQUEST_ROUND_SCREEN,
          "round request type");

    memset(&pack, 0, sizeof(pack));
    g_ImageBlockBuffer = pack.bytes;
    g_AssetLoadState = 1;
    s_loadResult = 16;
    LoadRoundAssets();
    Check(s_lastAssetId == ASSET_TIME_ATTACK_ROUND_SCREEN,
          "time-attack round asset id");
    Check(g_AssetLoadState == 2 && g_AssetBlockPtr2 == pack.bytes + 16,
          "round screen advances to voice bank");

    ((s32 *)(void *)(pack.bytes + 16))[0] = 123;
    ((s32 *)(void *)(pack.bytes + 16))[1] = 20;
    ((s32 *)(void *)(pack.bytes + 16))[2] = 40;
    s_loadResult = 1;
    LoadRoundAssets();
    Check(s_lastAssetId == ASSET_VOICE_BANK, "round voice asset id");
    Check(g_SharedAssetWord0 == 123, "round shared header word");
    Check(g_AssetBlockPtr == pack.bytes + 36 &&
              g_AssetSubBlockPtr == pack.bytes + 56,
          "round voice offsets relocated");
    Check(g_AssetLoadState == 0, "round load completes");

    if (s_failures != 0) return 1;
    puts("asset requests acknowledge and install their BGM blocks");
    return 0;
}
