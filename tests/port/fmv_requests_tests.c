#include "common.h"
#include "game/asset.h"
#include "game/race.h"
#include "game/state.h"

#include <stdio.h>

AssetRequestType g_AssetRequestType;
s32 g_AssetLoadState;
s32 g_PendingCarModelIndex;
u8 *g_AssetLoadCursor;
s32 g_CourseIndex;
s32 g_GrandPrixClass;
s16 g_SeriesSelection;
GameCdLoadEntry g_StreamCdEntries[11];
GameCdLoadEntry *g_StreamLoc;
u32 g_StreamSectorCount;
u32 g_StreamSectorLimit;

static s32 s_beginReturnScene;
static s32 s_loadResult;
static s32 s_loadAssetIndex;
static void *s_loadDestination;
static s32 s_installedAssetIndex;
static s32 s_installedSeriesCamera;
static s32 s_enableCdResult;
static s32 s_loaderCall;
static s32 s_loaderArgument;
static s32 s_failures;

enum {
    LOADER_NONE,
    LOADER_BOOT,
    LOADER_SAVE,
    LOADER_BGM,
    LOADER_CAR_SELECT,
    LOADER_CAR_MODEL,
    LOADER_UPGRADED_CAR,
    LOADER_OPTION,
    LOADER_ROUND,
    LOADER_RACE,
    LOADER_GP_SCREEN,
    LOADER_COURSE
};

void BeginFmv(s32 returnScene) { s_beginReturnScene = returnScene; }
s32 LoadAsset(s32 assetIndex, void *destination) {
    s_loadAssetIndex = assetIndex;
    s_loadDestination = destination;
    return s_loadResult;
}
void InstallTrackRuntimeAssetPack(s32 assetIndex, s32 useSeriesCamera) {
    s_installedAssetIndex = assetIndex;
    s_installedSeriesCamera = useSeriesCamera;
}
s32 EnableCdAudioMode(void) { return s_enableCdResult; }

void LoadBootAssets(void) { s_loaderCall = LOADER_BOOT; }
void LoadSaveScreenAssets(void) { s_loaderCall = LOADER_SAVE; }
void LoadSelectBgmAssets(void) { s_loaderCall = LOADER_BGM; }
void LoadCarSelectAssets(void) { s_loaderCall = LOADER_CAR_SELECT; }
void LoadCarModel(s32 model) {
    s_loaderCall = LOADER_CAR_MODEL; s_loaderArgument = model;
}
void LoadUpgradedCarModel(s32 model) {
    s_loaderCall = LOADER_UPGRADED_CAR; s_loaderArgument = model;
}
void LoadOptionScreenAssets(void) { s_loaderCall = LOADER_OPTION; }
void LoadRoundAssets(void) { s_loaderCall = LOADER_ROUND; }
void LoadRaceAssets(void) { s_loaderCall = LOADER_RACE; }
void LoadGrandPrixScreen(void) { s_loaderCall = LOADER_GP_SCREEN; }
void LoadCourseAssets(void) { s_loaderCall = LOADER_COURSE; }

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void TestFmvSelection(void) {
    s32 i;

    for (i = 0; i < 11; i++) g_StreamCdEntries[i].size = (u32)(100 + i);

    BeginIntroFmv(3);
    Check(s_beginReturnScene == 3, "intro return scene");
    Check(g_StreamLoc == &g_StreamCdEntries[0] &&
              g_StreamSectorCount == 100 && g_StreamSectorLimit == 200,
          "intro stream selection");

    g_SeriesSelection = 0;
    g_GrandPrixClass = 2;
    BeginClassFmv(7);
    Check(s_beginReturnScene == 7, "class return scene");
    Check(g_StreamLoc == &g_StreamCdEntries[3] &&
              g_StreamSectorCount == 103 && g_StreamSectorLimit == 206,
          "Grand Prix class stream selection");

    g_SeriesSelection = 1;
    BeginClassFmv(8);
    Check(g_StreamLoc == &g_StreamCdEntries[7] &&
              g_StreamSectorCount == 107 && g_StreamSectorLimit == 214,
          "Extra Grand Prix class stream selection");

    g_GrandPrixClass = -4;
    BeginClassFmv(8);
    Check(g_StreamLoc == &g_StreamCdEntries[5],
          "negative class clamps to the first class stream");
    g_GrandPrixClass = 9;
    BeginClassFmv(8);
    Check(g_StreamLoc == &g_StreamCdEntries[8],
          "high class clamps to the last class stream");

    BeginEndingFmv(0x21);
    Check(s_beginReturnScene == 0x21, "ending return scene");
    Check(g_StreamLoc == &g_StreamCdEntries[10] &&
              g_StreamSectorCount == 110 && g_StreamSectorLimit == 440,
          "ending stream selection");
}

static void TestTrackDataLoad(void) {
    u8 destination[64];
    s32 expectedAsset = ASSET_TRACK_2ND_BASE + 3 * 8 + 2 * 2;

    g_GrandPrixClass = 3;
    g_CourseIndex = 2;
    g_AssetLoadCursor = destination;
    g_AssetLoadState = 1;
    s_loadResult = 0;
    LoadTrackDataAssets();
    Check(g_AssetLoadState == 1, "pending track data holds phase");
    Check(s_loadAssetIndex == expectedAsset &&
              s_loadDestination == destination,
          "track data asset request");

    s_loadResult = 1;
    LoadTrackDataAssets();
    Check(g_AssetLoadState == 2 &&
              s_installedAssetIndex == expectedAsset &&
              s_installedSeriesCamera == 0,
          "track data pack installation");

    s_enableCdResult = 0;
    LoadTrackDataAssets();
    Check(g_AssetLoadState == 2, "pending CD mode holds track data phase");
    s_enableCdResult = 1;
    LoadTrackDataAssets();
    Check(g_AssetLoadState == 0, "CD mode completes track data load");
}

static void CheckDispatch(AssetRequestType request, s32 expectedLoader) {
    g_AssetRequestType = request;
    g_AssetLoadState = 1;
    s_loaderCall = LOADER_NONE;
    ServiceAssetLoad();
    Check(s_loaderCall == expectedLoader, "asset request dispatch");
}

static void TestAssetDispatch(void) {
    CheckDispatch(ASSET_REQUEST_BOOT, LOADER_BOOT);
    CheckDispatch(ASSET_REQUEST_SAVE_SCREEN, LOADER_SAVE);
    CheckDispatch(ASSET_REQUEST_SELECT_BGM, LOADER_BGM);
    CheckDispatch(ASSET_REQUEST_CAR_SELECT, LOADER_CAR_SELECT);
    g_PendingCarModelIndex = 12;
    CheckDispatch(ASSET_REQUEST_CAR_MODEL, LOADER_CAR_MODEL);
    Check(s_loaderArgument == 12, "car model dispatch argument");
    g_PendingCarModelIndex = 13;
    CheckDispatch(ASSET_REQUEST_UPGRADED_CAR_MODEL, LOADER_UPGRADED_CAR);
    Check(s_loaderArgument == 13, "upgraded car dispatch argument");
    CheckDispatch(ASSET_REQUEST_OPTION_SCREEN, LOADER_OPTION);
    CheckDispatch(ASSET_REQUEST_ROUND_SCREEN, LOADER_ROUND);
    CheckDispatch(ASSET_REQUEST_RACE, LOADER_RACE);
    CheckDispatch(ASSET_REQUEST_GRAND_PRIX_SCREEN, LOADER_GP_SCREEN);
    CheckDispatch(ASSET_REQUEST_COURSE, LOADER_COURSE);

    g_AssetRequestType = ASSET_REQUEST_INVALID;
    g_AssetLoadState = 1;
    s_loaderCall = LOADER_NONE;
    ServiceAssetLoad();
    Check(s_loaderCall == LOADER_NONE, "invalid request dispatch does nothing");
    g_AssetRequestType = ASSET_REQUEST_IDLE;
    ServiceAssetLoad();
    Check(s_loaderCall == LOADER_NONE, "idle request dispatch does nothing");

    g_AssetRequestType = ASSET_REQUEST_TRACK_DATA;
    g_GrandPrixClass = 1;
    g_CourseIndex = 3;
    s_loadResult = 0;
    ServiceAssetLoad();
    Check(s_loadAssetIndex == ASSET_TRACK_2ND_BASE + 14,
          "track data request dispatch");

    g_AssetLoadState = 0;
    s_loaderCall = LOADER_NONE;
    ServiceAssetLoad();
    Check(s_loaderCall == LOADER_NONE, "idle loader dispatch does nothing");
}

int main(void) {
    TestFmvSelection();
    TestTrackDataLoad();
    TestAssetDispatch();

    if (s_failures != 0) return 1;
    puts("FMV and asset requests select their exact streams and loaders");
    return 0;
}
