#include "common.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/race.h"
#include "game/track_camera_internal.h"

#include <stdio.h>
#include <string.h>

AssetRequestType g_AssetRequestType;
s32 g_AssetLoadState;
u8 *g_AssetBlockPtr;
u8 *g_AssetBlockPtr2;
u8 *g_AssetLoadCursor;
u8 *g_AssetSubBlockPtr;
u8 *g_AssetBase;
u8 *g_ImageBlockBuffer;
s32 g_SharedAssetWord0;
s32 g_PlayerCarIndex;
CarEntry *g_CarTable;
GameCarSpec *g_CarSpec;
s32 g_CourseIndex;
s32 g_GrandPrixClass;
s16 g_GrandPrixSeries;
TrackTextureShadowRow *g_TrackTextureShadow;

static s32 s_loadResult;
static s32 s_loadAssetIndex;
static void *s_loadDestination;
static s32 s_pollResult;
static s32 s_audioSlot;
static void *s_audioHeader;
static void *s_audioBody;
static u16 *s_audioSequence;
static s32 s_renderCarAsset;
static s32 s_uploadCount;
static GameImageAssetHeaderWord *s_uploads[5];
static s32 s_trackIdentity;
static s32 s_enableCdResult;
static s32 s_resetCdCalls;
static s32 s_failures;

s32 LoadAsset(s32 assetIndex, void *destination) {
    s_loadAssetIndex = assetIndex;
    s_loadDestination = destination;
    return s_loadResult;
}

s32 StartAudioSlotLoad(s32 slot, u8 *header, u8 *body, u16 *sequence) {
    s_audioSlot = slot;
    s_audioHeader = header;
    s_audioBody = body;
    s_audioSequence = sequence;
    return 1;
}

s32 PollAudioSlotLoad(void) { return s_pollResult; }
s32 GetCarAssetIndex(s32 model, s32 grade) { return model * 10 + grade; }
void GameRenderWorldSetTrackCarAsset(s32 asset) { s_renderCarAsset = asset; }
void UploadImageAsset(GameImageAssetHeaderWord *asset) {
    s_uploads[s_uploadCount++] = asset;
}
void UploadImageBlock(GameImageAssetHeaderWord *asset) {
    s_uploads[s_uploadCount++] = asset;
}
void StoreTeamLogoImage(void *destination) { (void)destination; }
void ResetTrackTextureSwap(void) {}
void TrackAssetIdentitySet(s32 assetIndex) { s_trackIdentity = assetIndex; }
void SetTrackRenderTable(struct TrackRenderTable *table) { (void)table; }
void SetEnvPaletteTable(struct EnvironmentPalette *table) { (void)table; }
void SetEnvironmentScript(u32 *script) { (void)script; }
void RegisterModelBank(ModelBankHeader *base, s32 index) {
    (void)base; (void)index;
}
void InstallTrackPoints(struct TrackPointTable *table) { (void)table; }
void RegisterCourseModels(CourseModelAssetHeader *base) { (void)base; }
void InstallTerrainCellData(void *data) { (void)data; }
void SetCourseObjects(struct CourseObjectTable *table) { (void)table; }
void InstallTrackEventData(struct TrackEventData *data) { (void)data; }
void SelectTrackCameraTable(TrackCameraTable *table, s32 useSeriesCamera) {
    (void)table; (void)useSeriesCamera;
}
s32 EnableCdAudioMode(void) { return s_enableCdResult; }
void ResetCdAudioState(void) { s_resetCdCalls++; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestRequestHandshake(void) {
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(RequestRaceAssets() == 1, "new race request pending");
    Check(g_AssetRequestType == ASSET_REQUEST_RACE,
          "new race request type");
    Check(g_AssetLoadState == 1, "new race request starts phase one");
    Check(RequestRaceAssets() == 1, "busy race request remains pending");
    g_AssetLoadState = 0;
    Check(RequestRaceAssets() == 0, "completed race request acknowledged");
    Check(g_AssetRequestType == ASSET_REQUEST_IDLE,
          "acknowledged race request becomes idle");
}

static void TestVoiceAndCarPhases(void) {
    u8 source[16];
    u8 destination[512];
    GameSceneAssetHeader *pack = (GameSceneAssetHeader *)destination;
    CarEntry cars[2];
    s32 i;

    for (i = 0; i < 16; i++) source[i] = (u8)(i + 1);
    memset(destination, 0, sizeof(destination));
    g_AssetBlockPtr = source;
    g_AssetLoadCursor = destination;
    g_AssetSubBlockPtr = source + 12;
    g_SharedAssetWord0 = 8;
    g_AssetLoadState = 1;
    LoadRaceAssets();
    Check(memcmp(source, destination, 8) == 0, "voice header copied");
    Check(g_AssetLoadCursor == destination + 8, "voice cursor advanced");
    Check(g_AssetLoadState == 2 && s_audioSlot == 2,
          "voice audio load started");

    s_pollResult = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 2, "pending voice audio holds phase");
    s_pollResult = 1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 3, "voice audio advances to car phase");

    memset(destination, 0, sizeof(destination));
    pack->offsets[0] = 64;
    pack->offsets[1] = 128;
    pack->offsets[2] = 160;
    pack->offsets[3] = 192;
    pack->offsets[4] = 224;
    *(u16 *)(void *)(destination + 160) = 7;
    memset(cars, 0, sizeof(cars));
    cars[1].modelVariant = 3;
    g_CarTable = cars;
    g_PlayerCarIndex = 1;
    g_AssetLoadCursor = destination;
    s_loadResult = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 3, "pending car pack holds phase");
    Check(s_renderCarAsset == 13 && s_loadAssetIndex == 37,
          "car asset index selected");

    s_loadResult = 1;
    s_uploadCount = 0;
    LoadRaceAssets();
    Check(g_CarSpec == (GameCarSpec *)(void *)(destination + 64),
          "car spec installed");
    Check(s_audioSlot == 3 && s_audioHeader == destination + 128 &&
              s_audioBody == destination + 192 &&
              s_audioSequence == (u16 *)(void *)(destination + 160),
          "car audio blocks installed");
    Check(s_uploadCount == 1 &&
              s_uploads[0] == (GameImageAssetHeaderWord *)(void *)(destination + 224),
          "car image uploaded");
    Check(g_AssetLoadCursor == destination + 192 && g_AssetLoadState == 4,
          "car phase advances to audio wait");
}

static void TestTrackPhases(void) {
    u8 storage[TRACK_TEXTURE_SHADOW_SIZE + 1024];
    GameSceneAssetHeader *pack = (GameSceneAssetHeader *)storage;
    s32 i;

    memset(storage, 0, sizeof(storage));
    for (i = 0; i < 11; i++) pack->offsets[i] = 128 + i * 32;
    g_CourseIndex = 2;
    g_GrandPrixClass = 3;
    g_AssetLoadCursor = storage;
    g_AssetLoadState = 5;
    s_loadResult = 1;
    s_uploadCount = 0;
    LoadRaceAssets();
    Check(s_loadAssetIndex == ASSET_TRACK_1ST_BASE + 28,
          "track texture asset index");
    Check(s_uploadCount == 5, "all track texture blocks uploaded");
    Check(g_TrackTextureShadow == (TrackTextureShadowRow *)(void *)storage,
          "track texture shadow installed");
    Check(g_AssetLoadCursor == storage + TRACK_TEXTURE_SHADOW_SIZE &&
              g_AssetLoadState == 6,
          "track textures advance to runtime data");

    pack = (GameSceneAssetHeader *)g_AssetLoadCursor;
    memset(pack, 0, 512);
    for (i = 0; i < 11; i++) pack->offsets[i] = 128 + i * 32;
    LoadRaceAssets();
    Check(s_loadAssetIndex == ASSET_TRACK_2ND_BASE + 28,
          "track runtime asset index");
    Check(s_trackIdentity == s_loadAssetIndex && g_AssetLoadState == 7,
          "track runtime data installed");

    s_enableCdResult = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 7, "pending CD mode holds final phase");
    s_enableCdResult = 1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0, "CD mode completes race load");
}

static void TestRaceStartAndCourseRequests(void) {
    u8 storage[64];

    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    s_resetCdCalls = 0;
    Check(RequestRaceStart() == 1, "new race start request pending");
    Check(g_AssetRequestType == ASSET_REQUEST_GRAND_PRIX_SCREEN &&
              g_AssetLoadState == 1 && s_resetCdCalls == 1,
          "race start request initializes loader");
    g_AssetLoadState = 0;
    Check(RequestRaceStart() == 0, "race start request acknowledged");

    g_GrandPrixSeries = 2;
    g_GrandPrixClass = 3;
    g_ImageBlockBuffer = storage;
    g_AssetLoadState = 1;
    s_loadResult = 16;
    LoadGrandPrixScreen();
    Check(s_loadAssetIndex == ASSET_ROUND_SCREEN_BASE + 15,
          "Grand Prix screen asset index");
    Check(s_loadDestination == storage && g_AssetLoadState == 0,
          "Grand Prix screen completes after load");

    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(RequestTrackLoad() == 1, "new course request pending");
    Check(g_AssetRequestType == ASSET_REQUEST_COURSE &&
              g_AssetLoadState == 1,
          "course request initializes loader");
    g_AssetLoadState = 0;
    Check(RequestTrackLoad() == 0, "course request acknowledged");

    g_CourseIndex = 1;
    g_GrandPrixClass = 2;
    g_AssetBase = storage;
    g_AssetLoadState = 1;
    s_loadResult = 20;
    LoadCourseAssets();
    Check(s_loadAssetIndex == ASSET_TRACK_1ST_BASE + 18,
          "standalone course asset index");
    Check(s_loadDestination == storage && g_ImageBlockBuffer == storage + 20 &&
              g_AssetLoadState == 0,
          "standalone course load publishes trailing image buffer");
}

int main(void) {
    TestRequestHandshake();
    TestVoiceAndCarPhases();
    TestTrackPhases();
    TestRaceStartAndCourseRequests();

    if (s_failures != 0) return 1;
    puts("race asset loading advances through each completed phase");
    return 0;
}
