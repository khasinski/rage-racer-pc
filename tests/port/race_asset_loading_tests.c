#include "common.h"
#include "game/asset.h"
#include "game/audio.h"
#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "game/track_camera_internal.h"

#include <stdio.h>
#include <string.h>

AssetRequestType g_AssetRequestType;
s32 g_AssetLoadState;
s32 g_AssetLoadFailed;
u8 *g_AssetBlockPtr;
size_t g_AssetBlockSize;
u8 *g_AssetBlockPtr2;
size_t g_AssetBlock2Size;
u8 *g_AssetLoadCursor;
u8 *g_AssetSubBlockPtr;
size_t g_AssetSubBlockSize;
u8 *g_AssetBase;
u8 *g_ImageBlockBuffer;
size_t g_ImageBlockSize;
s32 g_RaceVoiceHeaderSize;
s32 g_PlayerCarIndex;
CarEntry *g_CarTable;
GameCarSpec *g_CarSpec;
s32 g_CourseIndex;
s32 g_GrandPrixClass;
s16 g_GrandPrixSeries;
TrackTextureShadowRow *g_TrackTextureShadow;
TrackRenderTable *g_TrackRenderTable;
EnvironmentPalette *g_EnvPaletteTable;
CourseObject *g_CourseObjects;
s32 g_CourseObjectCount;

static s32 s_loadResult;
static s32 s_loadAssetIndex;
static void *s_loadDestination;
static s32 s_pollResult;
static s32 s_audioSlot;
static void *s_audioHeader;
static void *s_audioBody;
static u16 *s_audioSequence;
static size_t s_audioHeaderSize;
static size_t s_audioBodySize;
static size_t s_audioAuxiliarySize;
static s32 s_startAudioResult = 1;
static s32 s_renderCarAsset;
static s32 s_uploadCount;
static GameImageAssetHeaderWord *s_uploads[5];
static size_t s_uploadSizes[5];
static s32 s_validationCount;
static s32 s_validationFailureAt = -1;
static void *s_teamLogoSource;
static s32 s_textureResetCalls;
static s32 s_trackIdentity;
static s32 s_installCount;
static void *s_installs[8];
static s32 s_seriesCamera;
static s32 s_cameraTableValid = 1;
static s32 s_cameraSelectionResult = 1;
static s32 s_enableCdResult;
static s32 s_resetCdCalls;
static s32 s_failures;
static size_t s_assetRoom = SIZE_MAX;
static s32 s_invalidCarAssetIndex;

size_t PortAssetRoomAt(const void *at) {
    (void)at;
    return s_assetRoom;
}

s32 LoadAsset(s32 assetIndex, void *destination) {
    s_loadAssetIndex = assetIndex;
    s_loadDestination = destination;
    return s_loadResult;
}

s32 StartAudioSlotLoad(s32 slot, const AudioSlotAsset *asset) {
    s_audioSlot = slot;
    s_audioHeader = asset->vabHeader;
    s_audioHeaderSize = asset->vabHeaderSize;
    s_audioBody = asset->vabBody;
    s_audioBodySize = asset->vabBodySize;
    s_audioSequence = asset->auxiliaryData;
    s_audioAuxiliarySize = asset->auxiliarySize;
    return s_startAudioResult;
}

s32 PollAudioSlotLoad(void) { return s_pollResult; }
s32 GetCarAssetIndex(s32 model, s32 grade) {
    return s_invalidCarAssetIndex ? -1 : model * 10 + grade;
}
void GameRenderWorldSetTrackCarAsset(s32 asset) { s_renderCarAsset = asset; }
s32 UploadImageAsset(GameImageAssetHeaderWord *asset, size_t size) {
    s32 index = s_uploadCount++;
    s_uploads[index] = asset;
    s_uploadSizes[index] = size;
    return 1;
}
s32 UploadImageEntry(GameImageEntryHeader *entry, size_t size) {
    s32 index = s_uploadCount++;
    s_uploads[index] = (GameImageAssetHeaderWord *)entry;
    s_uploadSizes[index] = size;
    return 1;
}
s32 IsValidImageAsset(const GameImageAssetHeaderWord *asset, size_t size) {
    s32 index = s_validationCount++;
    (void)asset;
    (void)size;
    return index != s_validationFailureAt;
}
s32 IsValidImageEntry(const GameImageEntryHeader *entry, size_t size) {
    s32 index = s_validationCount++;
    (void)entry;
    (void)size;
    return index != s_validationFailureAt;
}
void StoreTeamLogoImage(void *source) { s_teamLogoSource = source; }
void ResetTrackTextureSwap(void) { s_textureResetCalls++; }
void TrackAssetIdentitySet(s32 assetIndex) { s_trackIdentity = assetIndex; }
s32 IsValidModelBankAsset(const ModelBankHeader *base, size_t size) {
    (void)base;
    (void)size;
    return 1;
}
s32 IsValidCourseModelAsset(const CourseModelAssetHeader *base, size_t size) {
    (void)base;
    (void)size;
    return 1;
}
s32 IsValidTerrainCellAsset(const void *data, size_t size) {
    (void)data;
    (void)size;
    return 1;
}
s32 IsValidEnvironmentScript(const struct GameEnvironmentScript *script,
                             size_t size) {
    (void)script;
    (void)size;
    return 1;
}
s32 IsValidTrackPointAsset(const struct TrackPointTable *table, size_t size) {
    (void)table;
    (void)size;
    return 1;
}
s32 IsValidTrackEventAsset(const struct TrackEventData *data, size_t size) {
    (void)data;
    (void)size;
    return 1;
}
s32 IsValidTrackCameraTable(const TrackCameraTable *table, size_t size,
                            s32 useSeriesCamera) {
    (void)table;
    (void)size;
    (void)useSeriesCamera;
    return s_cameraTableValid;
}
s32 SetEnvironmentScript(struct GameEnvironmentScript *script, size_t size) {
    (void)size;
    s_installs[s_installCount++] = script;
    return 1;
}
s32 RegisterModelBank(ModelBankHeader *base, size_t size, s32 index) {
    (void)size;
    (void)index;
    s_installs[s_installCount++] = base;
    return 1;
}
s32 InstallTrackPoints(struct TrackPointTable *table, size_t size) {
    (void)size;
    s_installs[s_installCount++] = table;
    return 1;
}
s32 RegisterCourseModels(CourseModelAssetHeader *base, size_t size) {
    (void)size;
    s_installs[s_installCount++] = base;
    return 1;
}
s32 InstallTerrainCellData(void *data, size_t size) {
    (void)size;
    s_installs[s_installCount++] = data;
    return 1;
}
s32 InstallTrackEventData(struct TrackEventData *data, size_t size) {
    (void)size;
    s_installs[s_installCount++] = data;
    return 1;
}
s32 SelectTrackCameraTable(TrackCameraTable *table, size_t size,
                           s32 useSeriesCamera) {
    (void)size;
    s_installs[s_installCount++] = table;
    s_seriesCamera = useSeriesCamera;
    return s_cameraSelectionResult;
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
    u8 destination[4096];
    s32 *offsets = (s32 *)(void *)destination;
    CarEntry cars[2];
    const s32 specificationOffset = 64;
    const s32 audioHeaderOffset =
        specificationOffset + (s32)sizeof(GameCarSpec);
    const s32 audioSequenceOffset = audioHeaderOffset + 64;
    const s32 audioBodyOffset =
        audioSequenceOffset + ENGINE_SOUND_PARAMETER_TABLE_SIZE;
    const s32 imageOffset = audioBodyOffset + 64;
    s32 i;

    for (i = 0; i < 16; i++) source[i] = (u8)(i + 1);
    memset(destination, 0, sizeof(destination));
    g_AssetBlockPtr = source;
    g_AssetBlockSize = sizeof(source);
    g_AssetLoadCursor = destination;
    g_AssetSubBlockPtr = source + 12;
    g_AssetSubBlockSize = 4;
    g_RaceVoiceHeaderSize = 10;
    g_AssetLoadState = 1;
    s_assetRoom = sizeof(destination);
    s_startAudioResult = -1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              g_AssetLoadCursor == destination,
          "failed race-voice transfer cancels loading without advancing");

    g_AssetLoadState = 1;
    s_startAudioResult = 1;
    s_assetRoom = 9;
    memset(destination, 0, 10);
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              destination[0] == 0,
          "voice header exceeding destination storage is rejected");

    g_AssetLoadState = 1;
    s_assetRoom = sizeof(destination);
    g_AssetBlockSize = 9;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed(),
          "voice header exceeding its source block is rejected");

    g_AssetLoadState = 1;
    g_AssetBlockSize = sizeof(source);
    s_startAudioResult = 1;
    LoadRaceAssets();
    Check(memcmp(source, destination, 10) == 0,
          "entire byte-sized voice header copied");
    Check(g_AssetLoadCursor == destination + 10, "voice cursor advanced");
    Check(g_AssetLoadState == 2 && s_audioSlot == 2,
          "voice audio load started");
    Check(s_audioHeaderSize == 10 && s_audioBodySize == 4,
          "voice audio load keeps both source bounds");

    s_pollResult = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 2, "pending voice audio holds phase");
    s_pollResult = 1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 3, "voice audio advances to car phase");

    g_PlayerCarIndex = GAME_CAR_COUNT;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed(),
          "out-of-range player car rejects the race asset request");
    g_AssetLoadState = 3;

    memset(destination, 0, sizeof(destination));
    offsets[0] = specificationOffset;
    offsets[1] = audioHeaderOffset;
    offsets[2] = audioSequenceOffset;
    offsets[3] = audioBodyOffset;
    offsets[4] = imageOffset;
    *(u16 *)(void *)(destination + audioSequenceOffset) = 7;
    memset(cars, 0, sizeof(cars));
    cars[1].modelVariant = 3;
    g_CarTable = NULL;
    g_PlayerCarIndex = 1;
    s_loadAssetIndex = -1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1,
          "missing car table rejects the race car pack before disc access");

    g_AssetLoadState = 3;
    g_CarTable = cars;
    s_invalidCarAssetIndex = 1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1,
          "invalid car variant rejects the race car pack before disc access");

    g_AssetLoadState = 3;
    s_invalidCarAssetIndex = 0;
    g_AssetLoadCursor = destination;
    s_renderCarAsset = -1;
    s_loadResult = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 3, "pending car pack holds phase");
    Check(s_renderCarAsset == -1 && s_loadAssetIndex == 37,
          "pending car pack selects only its disc asset");

    s_loadResult = sizeof(destination);
    offsets[1] = specificationOffset + (s32)sizeof(GameCarSpec) - 1;
    s_uploadCount = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && s_renderCarAsset == -1 &&
              s_uploadCount == 0,
          "truncated car specification rejects the race car pack");

    g_AssetLoadState = 3;
    offsets[1] = audioHeaderOffset;
    offsets[3] = audioSequenceOffset +
                 ENGINE_SOUND_PARAMETER_TABLE_SIZE - 1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && s_renderCarAsset == -1 &&
              s_uploadCount == 0,
          "truncated engine parameter table rejects the race car pack");

    g_AssetLoadState = 3;
    offsets[3] = audioBodyOffset;
    offsets[4] = offsets[3];
    s_uploadCount = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && s_renderCarAsset == -1 &&
              s_uploadCount == 0,
          "overlapping car blocks cancel installation");

    g_AssetLoadState = 3;
    offsets[4] = imageOffset;
    s_startAudioResult = -1;
    s_renderCarAsset = -1;
    g_CarSpec = NULL;
    s_uploadCount = 0;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && s_renderCarAsset == -1 &&
              g_CarSpec == NULL && s_uploadCount == 0,
          "failed engine transfer publishes no car pack state");

    g_AssetLoadState = 3;
    s_startAudioResult = 1;
    s_uploadCount = 0;
    LoadRaceAssets();
    Check(s_renderCarAsset == 13, "loaded car asset is selected for rendering");
    Check(g_CarSpec == GetGameCarSpec(destination + specificationOffset),
          "car spec installed");
    Check(s_audioSlot == 3 &&
              s_audioHeader == destination + audioHeaderOffset &&
              s_audioBody == destination + audioBodyOffset &&
              s_audioSequence ==
                  (u16 *)(void *)(destination + audioSequenceOffset) &&
              s_audioHeaderSize ==
                  (size_t)(audioSequenceOffset - audioHeaderOffset) &&
              s_audioBodySize == (size_t)(imageOffset - audioBodyOffset) &&
              s_audioAuxiliarySize == ENGINE_SOUND_PARAMETER_TABLE_SIZE,
          "car audio blocks installed");
    Check(s_uploadCount == 1 &&
              s_uploads[0] ==
                  (GameImageAssetHeaderWord *)(void *)(destination +
                                                        imageOffset),
          "car image uploaded");
    Check(g_AssetLoadCursor == destination + audioBodyOffset &&
              g_AssetLoadState == 4,
          "car phase advances to audio wait");
}

static void TestTrackPhases(void) {
    u8 storage[TRACK_TEXTURE_SHADOW_SIZE + 2048];
    GameSceneAssetHeader *pack = (GameSceneAssetHeader *)storage;
    static const s32 runtimeInstallSlots[8] = {2, 3, 4, 5, 6, 7, 9, 10};
    s32 i;

    memset(storage, 0, sizeof(storage));
    for (i = 0; i < 11; i++) pack->offsets[i] = 128 + i * 32;
    g_CourseIndex = 2;
    g_GrandPrixClass = 3;
    g_AssetLoadCursor = storage;
    g_AssetLoadState = 5;
    g_GrandPrixClass = TRACK_CLASS_COUNT;
    s_loadAssetIndex = -1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1,
          "invalid track class rejects texture loading before disc access");

    g_AssetLoadState = 5;
    g_GrandPrixClass = 3;
    s_loadResult = sizeof(storage);
    s_uploadCount = 0;
    LoadRaceAssets();
    Check(s_loadAssetIndex == ASSET_TRACK_1ST_BASE + 28,
          "track texture asset index");
    Check(s_uploadCount == 5, "all track texture blocks uploaded");
    for (i = 0; i < 5; i++) {
        Check(s_uploads[i] ==
                  (GameImageAssetHeaderWord *)(void *)(storage +
                                                        pack->offsets[i]),
              "track texture block address");
    }
    Check(g_TrackTextureShadow == (TrackTextureShadowRow *)(void *)storage,
          "track texture shadow installed");
    Check(g_AssetLoadCursor == storage + TRACK_TEXTURE_SHADOW_SIZE &&
              g_AssetLoadState == 6,
          "track textures advance to runtime data");

    pack = (GameSceneAssetHeader *)g_AssetLoadCursor;
    memset(pack, 0, 1088);
    pack->offsets[0] = 64;
    pack->offsets[1] = 192;
    pack->offsets[2] = 448;
    for (i = 3; i < 11; i++) pack->offsets[i] = 512 + (i - 3) * 64;
    ((CourseObjectTable *)(void *)((u8 *)pack + pack->offsets[8]))->count = 2;
    s_loadResult = 1088;
    s_installCount = 0;
    s_seriesCamera = 0;
    g_AssetLoadState = 6;
    g_CourseIndex = TRACK_COURSE_COUNT;
    s_loadAssetIndex = -1;
    LoadRaceAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1 && s_installCount == 0,
          "invalid track slot rejects runtime loading before disc access");

    g_AssetLoadState = 6;
    g_CourseIndex = 2;
    LoadRaceAssets();
    Check(s_loadAssetIndex == ASSET_TRACK_2ND_BASE + 28,
          "track runtime asset index");
    Check(s_trackIdentity == s_loadAssetIndex && g_AssetLoadState == 7,
          "track runtime data installed");
    Check(s_installCount == 8 && s_seriesCamera == 1,
          "all runtime blocks use the series camera table");
    for (i = 0; i < 8; i++) {
        Check(s_installs[i] ==
                  (void *)((u8 *)pack + pack->offsets[runtimeInstallSlots[i]]),
              "track runtime block address");
    }
    Check(g_TrackRenderTable ==
              (TrackRenderTable *)(void *)((u8 *)pack + pack->offsets[0]) &&
              g_EnvPaletteTable ==
                  (EnvironmentPalette *)(void *)((u8 *)pack + pack->offsets[1]),
          "runtime render tables published");
    Check(g_CourseObjects ==
                  ((CourseObjectTable *)(void *)((u8 *)pack + pack->offsets[8]))
                      ->objects &&
              g_CourseObjectCount == 2,
          "runtime course objects published");

    s_installCount = 0;
    Check(InstallTrackRuntimeAssetPack(pack, 1088, s_loadAssetIndex, 0) == 1,
          "resident runtime pack is valid");
    Check(s_installCount == 8 && s_seriesCamera == 0,
          "scene loads install the default camera table");

    pack->offsets[1] = pack->offsets[0];
    s_installCount = 0;
    s_trackIdentity = -1;
    Check(InstallTrackRuntimeAssetPack(pack, 1088, 123, 0) == 0 &&
              s_installCount == 0 && s_trackIdentity == -1,
          "overlapping runtime blocks reject the pack before installation");
    pack->offsets[1] = 192;
    ((CourseObjectTable *)(void *)((u8 *)pack + pack->offsets[8]))->count = 4;
    Check(InstallTrackRuntimeAssetPack(pack, 1088, 123, 0) == 0 &&
              s_installCount == 0 && s_trackIdentity == -1,
          "oversized course-object table rejects the runtime pack");
    ((CourseObjectTable *)(void *)((u8 *)pack + pack->offsets[8]))->count = 2;

    pack->offsets[1] = pack->offsets[0] + 64;
    s_installCount = 0;
    Check(InstallTrackRuntimeAssetPack(pack, 1088, 123, 0) == 0 &&
              s_installCount == 0,
          "truncated render table rejects the runtime pack");
    pack->offsets[1] = 192;

    pack->offsets[2] = pack->offsets[1] + 128;
    s_installCount = 0;
    Check(InstallTrackRuntimeAssetPack(pack, 1088, 123, 0) == 0 &&
              s_installCount == 0,
          "truncated environment palettes reject the runtime pack");
    pack->offsets[2] = 448;

    s_cameraTableValid = 0;
    s_installCount = 0;
    s_trackIdentity = -1;
    g_TrackRenderTable = NULL;
    g_EnvPaletteTable = NULL;
    Check(InstallTrackRuntimeAssetPack(pack, 1088, 123, 0) == 0 &&
              s_installCount == 0 && s_trackIdentity == -1 &&
              g_TrackRenderTable == NULL && g_EnvPaletteTable == NULL,
          "invalid camera table rejects the pack before publication");
    s_cameraTableValid = 1;

    s_cameraSelectionResult = 0;
    s_installCount = 0;
    s_trackIdentity = -1;
    g_TrackRenderTable = NULL;
    g_EnvPaletteTable = NULL;
    g_CourseObjects = NULL;
    g_CourseObjectCount = -1;
    Check(InstallTrackRuntimeAssetPack(pack, 1088, 123, 0) == 0 &&
              s_installCount == 8 && s_trackIdentity == -1 &&
              g_TrackRenderTable == NULL && g_EnvPaletteTable == NULL &&
              g_CourseObjects == NULL && g_CourseObjectCount == -1,
          "failed final installer publishes no top-level track state");
    s_cameraSelectionResult = 1;

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

    g_GrandPrixSeries = 1;
    g_GrandPrixClass = 3;
    g_ImageBlockBuffer = storage;
    s_loadAssetIndex = -1;
    g_AssetLoadState = 0;
    LoadGrandPrixScreen();
    Check(s_loadAssetIndex == -1,
          "Grand Prix screen ignores inactive loader states");
    g_AssetLoadState = 1;
    s_loadResult = 0;
    LoadGrandPrixScreen();
    Check(g_AssetLoadState == 1,
          "Grand Prix screen waits for an incomplete load");
    s_loadResult = 16;
    LoadGrandPrixScreen();
    Check(s_loadAssetIndex == ASSET_ROUND_SCREEN_BASE + 9,
          "Grand Prix screen asset index");
    Check(s_loadDestination == storage && g_ImageBlockSize == 16 &&
              g_AssetLoadState == 0,
          "Grand Prix screen completes after load");

    g_AssetLoadState = 1;
    g_GrandPrixSeries = GRAND_PRIX_SERIES_COUNT;
    s_loadAssetIndex = -1;
    LoadGrandPrixScreen();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1,
          "invalid Grand Prix series rejects screen loading");

    g_AssetLoadState = 1;
    g_GrandPrixSeries = 0;
    g_GrandPrixClass = GRAND_PRIX_PRIZE_CLASS_COUNT;
    LoadGrandPrixScreen();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1,
          "invalid Grand Prix class rejects screen loading");

    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(RequestCourseTextureAssets() == 1,
          "new course-texture request pending");
    Check(g_AssetRequestType == ASSET_REQUEST_COURSE_TEXTURES &&
              g_AssetLoadState == 1,
          "course-texture request initializes loader");
    g_AssetLoadState = 0;
    Check(RequestCourseTextureAssets() == 0,
          "course-texture request acknowledged");

    g_CourseIndex = 1;
    g_GrandPrixClass = 2;
    g_AssetBase = storage;
    g_ImageBlockBuffer = NULL;
    g_ImageBlockSize = 99;
    s_loadAssetIndex = -1;
    g_AssetLoadState = 0;
    LoadCourseTextureAssets();
    Check(s_loadAssetIndex == -1,
          "course loader ignores inactive loader states");
    g_AssetLoadState = 1;
    s_loadResult = 0;
    LoadCourseTextureAssets();
    Check(g_AssetLoadState == 1 && g_ImageBlockBuffer == NULL,
          "course loader waits without publishing an incomplete pack");
    s_loadResult = 20;
    LoadCourseTextureAssets();
    Check(s_loadAssetIndex == ASSET_TRACK_1ST_BASE + 18,
          "standalone course asset index");
    Check(s_loadDestination == storage && g_ImageBlockBuffer == storage + 20 &&
              g_ImageBlockSize == 0 && g_AssetLoadState == 0,
          "standalone course load publishes trailing image buffer");

    g_AssetLoadState = 1;
    g_CourseIndex = TRACK_COURSE_COUNT;
    s_loadAssetIndex = -1;
    LoadCourseTextureAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1,
          "invalid standalone track slot is rejected before disc access");
}

static void TestResidentCourseInstallation(void) {
    static u8 storage[TRACK_TEXTURE_SHADOW_SIZE + 512];
    GameSceneAssetHeader *pack = (GameSceneAssetHeader *)storage;
    s32 i;

    memset(storage, 0, sizeof(storage));
    for (i = 0; i < 5; i++) pack->offsets[i] = 64 + i * 32;
    g_AssetBase = storage;
    g_AssetLoadCursor = NULL;
    Check(InstallTrackTextureAssetPack(g_AssetBase, 256) == 0 &&
              g_AssetLoadCursor == NULL,
          "truncated texture-shadow storage rejects the course pack");
    s_uploadCount = 0;
    s_validationCount = 0;
    s_validationFailureAt = -1;
    s_teamLogoSource = NULL;
    s_textureResetCalls = 0;
    Check(InstallTrackTextureAssetPack(g_AssetBase, sizeof(storage)) == 1,
          "resident course texture pack is valid");
    Check(s_uploadCount == 5, "resident course uploads every texture block");
    Check(s_uploadSizes[0] == 32 && s_uploadSizes[1] == 32 &&
              s_uploadSizes[2] == 32 && s_uploadSizes[3] == 32 &&
              s_uploadSizes[4] == sizeof(storage) - 192,
          "course uploader bounds every image sub-block");
    Check(s_teamLogoSource == storage, "resident course stores team logo");
    Check(g_TrackTextureShadow == (TrackTextureShadowRow *)(void *)storage,
          "resident course installs texture shadow");
    Check(g_AssetLoadCursor == storage + TRACK_TEXTURE_SHADOW_SIZE,
          "resident course publishes runtime pack cursor");
    Check(s_textureResetCalls == 1, "resident course resets texture swapping");

    pack->offsets[1] = pack->offsets[0];
    g_AssetLoadCursor = NULL;
    s_uploadCount = 0;
    s_textureResetCalls = 0;
    Check(InstallTrackTextureAssetPack(g_AssetBase, sizeof(storage)) == 0,
          "overlapping texture blocks reject the course pack");
    Check(s_uploadCount == 0 && s_textureResetCalls == 0 &&
              g_AssetLoadCursor == NULL,
          "invalid course pack publishes no texture state");
    pack->offsets[1] = 96;

    g_AssetLoadCursor = NULL;
    g_TrackTextureShadow = NULL;
    s_teamLogoSource = NULL;
    s_textureResetCalls = 0;
    s_uploadCount = 0;
    s_validationCount = 0;
    s_validationFailureAt = 2;
    Check(InstallTrackTextureAssetPack(g_AssetBase, sizeof(storage)) == 0,
          "invalid embedded image rejects the course pack");
    Check(g_AssetLoadCursor == NULL && g_TrackTextureShadow == NULL &&
              s_teamLogoSource == NULL && s_textureResetCalls == 0,
          "invalid embedded image publishes no course texture state");
    s_validationFailureAt = -1;

    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    Check(RequestTrackDataAssets() == 1, "new track data request pending");
    Check(g_AssetRequestType == ASSET_REQUEST_TRACK_DATA &&
              g_AssetLoadState == 1,
          "track data request initializes loader");
    g_AssetLoadState = 0;
    Check(RequestTrackDataAssets() == 0, "track data request acknowledged");

    g_AssetLoadState = 1;
    g_GrandPrixClass = 0;
    g_CourseIndex = TRACK_COURSE_COUNT;
    s_loadAssetIndex = -1;
    LoadTrackDataAssets();
    Check(g_AssetLoadState == 0 && AssetLoadHasFailed() &&
              s_loadAssetIndex == -1,
          "standalone runtime loader rejects a physical course selector");
}

int main(void) {
    TestRequestHandshake();
    TestVoiceAndCarPhases();
    TestTrackPhases();
    TestRaceStartAndCourseRequests();
    TestResidentCourseInstallation();

    if (s_failures != 0) return 1;
    puts("race asset loading advances through each completed phase");
    return 0;
}
