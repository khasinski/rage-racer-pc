#include "common.h"
#include "game/asset.h"
#include "game/audio.h"
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
s32 g_LoadBuffer[64];
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
static s32 s_randomValues[2];
static s32 s_randomCallCount;
static s32 s_selectModelBankCalls;
static void *s_lastLoadDestination;
static s32 s_loadBufferUploads;
static s32 s_audioSlot;
static u8 *s_audioHeader;
static u8 *s_audioBody;
static u16 *s_audioTable;
static s32 s_pollResult;
static s32 s_imageUploads;
static GameImageAssetHeaderWord *s_lastImageUpload;
static s32 s_storeImageCalls;
static s32 s_drawSyncCalls;

s32 LoadAsset(s32 slot, void *destination) {
    s_lastAssetId = slot;
    s_lastLoadDestination = destination;
    return s_loadResult;
}
void UploadLoadBufferImage(void) { s_loadBufferUploads++; }
s32 StartAudioSlotLoad(s32 slot, u8 *header, u8 *body, u16 *table) {
    s_audioSlot = slot;
    s_audioHeader = header;
    s_audioBody = body;
    s_audioTable = table;
    return 1;
}
s32 PollAudioSlotLoad(void) { return s_pollResult; }
void UploadImageAsset(GameImageAssetHeaderWord *data) {
    s_lastImageUpload = data;
    s_imageUploads++;
}
void StoreImage(Rect *rect, void *data) {
    (void)rect;
    (void)data;
    s_storeImageCalls++;
}
void DrawSync(long mode) {
    (void)mode;
    s_drawSyncCalls++;
}
s32 CloseLoadedAudioSlots(void) {
    s_closeCalls++;
    return 0;
}
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
s32 Random15(void) {
    s32 index = s_randomCallCount < 2 ? s_randomCallCount : 1;
    s_randomCallCount++;
    return s_randomValues[index];
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestBootAssetPhases(void) {
    u8 *loadBase = (u8 *)(void *)g_LoadBuffer;
    u8 *resourceBase;

    g_AssetLoadState = 1;
    s_loadResult = 0;
    s_loadBufferUploads = 0;
    LoadBootAssets();
    Check(g_AssetLoadState == 1 && s_loadBufferUploads == 0,
          "pending title screen holds boot phase");
    s_loadResult = 8;
    LoadBootAssets();
    Check(s_lastAssetId == ASSET_TITLE_SCREEN &&
              s_lastLoadDestination == loadBase &&
              g_AssetBlockPtr == loadBase + 8 &&
              g_AssetLoadState == 2 && s_loadBufferUploads == 1,
          "title screen advances boot loader");

    s_loadResult = 0;
    LoadBootAssets();
    Check(g_AssetLoadState == 2, "pending audio header holds boot phase");
    s_loadResult = 12;
    LoadBootAssets();
    Check(s_lastAssetId == ASSET_BOOT_AUDIO_HEADER &&
              s_lastLoadDestination == loadBase + 8 &&
              g_AssetLoadCursor == loadBase + 20 &&
              g_AssetLoadState == 3,
          "audio header advances boot loader");

    s_loadResult = 1;
    LoadBootAssets();
    Check(s_lastAssetId == ASSET_BOOT_AUDIO_BODY && s_audioSlot == 0 &&
              s_audioHeader == loadBase + 8 &&
              s_audioBody == loadBase + 20 && s_audioTable == NULL &&
              g_AssetLoadState == 4,
          "audio body starts boot audio slot");

    s_pollResult = 0;
    LoadBootAssets();
    Check(g_AssetLoadState == 4, "pending boot audio holds phase");
    s_pollResult = 1;
    LoadBootAssets();
    Check(g_AssetLoadState == 5, "ready boot audio advances phase");

    resourceBase = g_AssetLoadCursor;
    s_loadResult = 0;
    LoadBootAssets();
    Check(g_AssetLoadState == 5 && g_AssetLoadCursor == resourceBase,
          "pending resources hold boot phase");
    s_loadResult = 16;
    LoadBootAssets();
    Check(s_lastAssetId == ASSET_BOOT_RESOURCES &&
              g_AssetLoadCursor == resourceBase + 16 &&
              g_AssetLoadState == 6,
          "resources advance boot loader cursor");

    s_loadResult = 0;
    s_imageUploads = 0;
    LoadBootAssets();
    Check(g_AssetLoadState == 6 && s_imageUploads == 0,
          "pending car screen holds boot phase");
    s_loadResult = 1;
    g_TeamLogoClut[0] = 7;
    s_storeImageCalls = 0;
    s_drawSyncCalls = 0;
    LoadBootAssets();
    Check(s_lastAssetId == ASSET_BOOT_CAR_SCREEN &&
              s_lastImageUpload ==
                  (GameImageAssetHeaderWord *)(void *)(resourceBase + 16) &&
              g_AssetBase == resourceBase + 16 && g_AssetLoadState == 0,
          "car screen completes boot loader");
    Check(s_storeImageCalls == 2 && s_drawSyncCalls == 1,
          "car screen captures team logo image and CLUT");
    Check(g_TeamLogoClut[0] == 0, "boot car screen clears logo CLUT entry");
}

static void TestSaveScreenAssets(u8 *assetBase) {
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    s_resetCalls = 0;
    Check(RequestSaveScreenAssets() == 1, "new save request remains pending");
    Check(g_AssetRequestType == ASSET_REQUEST_SAVE_SCREEN &&
              g_AssetLoadState == 1 && s_resetCalls == 1,
          "save request starts asset loader");
    Check(RequestSaveScreenAssets() == 1,
          "busy save request remains pending");
    g_AssetLoadState = 0;
    Check(RequestSaveScreenAssets() == 0 &&
              g_AssetRequestType == ASSET_REQUEST_IDLE,
          "completed save request acknowledges");

    g_AssetBase = assetBase;
    g_ImageBlockBuffer = NULL;
    g_AssetLoadState = 1;
    s_loadResult = 0;
    LoadSaveScreenAssets();
    Check(g_AssetLoadState == 1 && g_ImageBlockBuffer == NULL,
          "pending save screen installs nothing");
    s_loadResult = 1;
    LoadSaveScreenAssets();
    Check(s_lastAssetId == ASSET_SAVE_SCREEN &&
              s_lastLoadDestination == assetBase &&
              g_ImageBlockBuffer == assetBase && g_AssetLoadState == 0,
          "save screen installs loaded image block");
}

static void TestSelectBgmRequests(void) {
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    s_resetCalls = 0;
    Check(RequestSelectBgmAssets() == 1, "new BGM request remains pending");
    Check(g_AssetRequestType == ASSET_REQUEST_SELECT_BGM,
          "new BGM request type");
    Check(g_AssetLoadState == 1, "BGM request closes audio before loading");
    Check(s_resetCalls == 1, "new BGM request resets CD audio");
    Check(RequestSelectBgmAssets() == 1, "busy BGM request remains pending");

    g_AssetLoadState = 0;
    Check(RequestSelectBgmAssets() == 0, "matching BGM request acknowledges");
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE,
          "acknowledged BGM request becomes idle");

    Check(RequestSelectBgmAssetsKeepAudioSlots() == 1,
          "keep-slots BGM request remains pending");
    Check(g_AssetLoadState == 2 && s_resetCalls == 2,
          "keep-slots request skips slot close phase but resets CD state");
}

int main(void) {
    union {
        max_align_t alignment;
        u8 bytes[128];
    } pack;
    GameSceneAssetHeader *header = GetSceneAssetHeader(pack.bytes);
    VoiceBankAssetHeader *voiceHeader;

    TestBootAssetPhases();
    TestSaveScreenAssets(pack.bytes);
    TestSelectBgmRequests();

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
    RequestOptionScreenAssets();
    Check(g_AssetRequestType == ASSET_REQUEST_OPTION_SCREEN &&
              g_AssetLoadState == 1 && s_resetCalls == 1,
          "busy OPTION request preserves loader state");
    g_AssetLoadState = 0;
    RequestOptionScreenAssets();
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE,
          "OPTION request acknowledged");

    memset(&pack, 0, sizeof(pack));
    ((OptionScreenAsset *)pack.bytes)->imageOffset = 48;
    g_AssetBase = pack.bytes;
    g_ImageBlockBuffer = NULL;
    g_AssetLoadState = 1;
    s_loadResult = 0;
    LoadOptionScreenAssets();
    Check(g_AssetLoadState == 1 && g_ImageBlockBuffer == NULL,
          "incomplete OPTION load installs nothing");
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
    s_randomValues[0] = 3;
    s_randomCallCount = 0;
    Check(RequestRoundAssets() == 1, "new round request pending");
    Check(g_GrandPrixClass == 3, "time-attack class randomized");
    Check(s_randomCallCount == 1, "time-attack class uses one random sample");
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

    voiceHeader = GetVoiceBankAssetHeader(pack.bytes + 16);
    voiceHeader->sharedHeaderSize = 123;
    voiceHeader->audioHeaderOffset = 20;
    voiceHeader->audioBodyOffset = 40;
    s_loadResult = 0;
    g_SharedAssetWord0 = -1;
    LoadRoundAssets();
    Check(g_AssetLoadState == 2 && g_SharedAssetWord0 == -1,
          "incomplete voice load installs nothing");
    s_loadResult = 1;
    LoadRoundAssets();
    Check(s_lastAssetId == ASSET_VOICE_BANK, "round voice asset id");
    Check(g_SharedAssetWord0 == 123, "round shared header word");
    Check(g_AssetBlockPtr == pack.bytes + 36 &&
              g_AssetSubBlockPtr == pack.bytes + 56,
          "round voice offsets relocated");
    Check(g_AssetLoadState == 0, "round load completes");

    g_GrandPrixMode = 0;
    g_CourseIndex = 3;
    g_MaxClassReached[0] = 4;
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    s_randomValues[0] = 0;
    s_randomValues[1] = 1;
    s_randomCallCount = 0;
    RequestRoundAssets();
    Check(g_GrandPrixClass == 3 && s_randomCallCount == 2,
          "oval rerolls classes below two");

    g_GrandPrixMode = 1;
    g_GrandPrixSeries = 1;
    g_GrandPrixClass = 4;
    g_AssetLoadState = 3;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    s_resetCalls = 0;
    Check(RequestRoundAssets() == 1, "busy GP round request restarts");
    Check(s_resetCalls == 1 && g_AssetLoadState == 1,
          "busy GP round request resets loader");
    g_ImageBlockBuffer = pack.bytes;
    s_loadResult = 8;
    LoadRoundAssets();
    Check(s_lastAssetId == ASSET_ROUND_SCREEN_BASE + 10,
          "Extra GP class round screen asset id");

    if (s_failures != 0) return 1;
    puts("asset requests acknowledge and install their BGM blocks");
    return 0;
}
