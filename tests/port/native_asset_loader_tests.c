#include "common.h"
#include "game/asset.h"

#include <stdio.h>

s16 g_CdLoadPhase;
s32 g_AssetLoadState;
AssetRequestType g_AssetRequestType;
GameCdLoadEntry g_AssetCdEntries[GAME_ASSET_COUNT];
s32 g_LoadBuffer[64];

static s32 s_modResult;
static s32 s_hostResult;
static s32 s_hostLoads;
static s32 s_patchCalls;
static size_t s_patchSize;
static s32 s_archiveLoads;
static s32 s_uploads;
static GameImageAssetHeaderWord *s_uploadedAsset;
static s32 s_requestCalls;
static s32 s_failures;

int ModAssetLoad(int index, void *destination, unsigned int originalSize) {
    (void)index;
    (void)destination;
    (void)originalSize;
    return s_modResult;
}

int HostLoadAsset(unsigned int byteOffset, unsigned int size,
                  void *destination) {
    (void)byteOffset;
    (void)size;
    (void)destination;
    s_hostLoads++;
    return s_hostResult;
}

void ModPatchTextures(int index, void *data, size_t size) {
    (void)index;
    (void)data;
    s_patchCalls++;
    s_patchSize = size;
}

int HostLoadArchiveIndex(void *entries, int count) {
    (void)entries;
    (void)count;
    s_archiveLoads++;
    return 1;
}

s32 UploadImageAsset(GameImageAssetHeaderWord *asset, size_t size) {
    (void)size;
    s_uploads++;
    s_uploadedAsset = asset;
    return 1;
}

s32 RequestAssetLoad(AssetRequestType request, s32 firstLoadState,
                     s32 resetCdAudio) {
    (void)request;
    (void)firstLoadState;
    (void)resetCdAudio;
    s_requestCalls++;
    return 7;
}

static void Check(s32 condition, const char *label) {
    if (!condition) {
        printf("FAIL %s\n", label);
        s_failures++;
    }
}

static void ResetCalls(void) {
    s_modResult = 0;
    s_hostResult = 0;
    s_hostLoads = 0;
    s_patchCalls = 0;
    s_patchSize = 0;
    s_archiveLoads = 0;
    s_uploads = 0;
    s_uploadedAsset = NULL;
    s_requestCalls = 0;
}

int main(void) {
    u8 destination[32];

    g_AssetCdEntries[3].position.sectorOffset = 123;
    g_AssetCdEntries[3].size = 24;

    ResetCalls();
    s_modResult = 20;
    Check(LoadAsset(3, destination) == 20,
          "mod asset returns its positive byte count");
    Check(s_hostLoads == 0 && s_patchCalls == 1 && s_patchSize == 20,
          "mod asset skips disc fallback and receives texture patches");

    ResetCalls();
    s_modResult = -1;
    s_hostResult = 24;
    Check(LoadAsset(3, destination) == 24,
          "negative mod result falls back to disc");
    Check(s_hostLoads == 1 && s_patchCalls == 1 && s_patchSize == 24,
          "disc fallback receives texture patches");

    ResetCalls();
    s_modResult = -1;
    s_hostResult = -1;
    Check(LoadAsset(3, destination) == 0,
          "negative backend result is normalized to failure");
    Check(s_patchCalls == 0, "failed asset is never patched");
    Check(LoadAsset(-1, destination) == 0 && s_hostLoads == 1,
          "invalid asset index does not reach a backend");

    ResetCalls();
    g_AssetCdEntries[ASSET_BOOT_LOGO].size = 16;
    InitAssetSystem();
    Check(s_archiveLoads == 1 && s_uploads == 0,
          "failed boot logo is not uploaded");

    ResetCalls();
    s_hostResult = 16;
    InitAssetSystem();
    Check(s_archiveLoads == 1 && s_uploads == 1 &&
              s_uploadedAsset == GetImageAssetHeaderWords(g_LoadBuffer),
          "loaded boot logo is uploaded");

    g_CdLoadPhase = 4;
    g_AssetLoadState = 5;
    g_AssetRequestType = ASSET_REQUEST_RACE;
    ResetAssetLoader();
    Check(g_CdLoadPhase == 0 && g_AssetLoadState == 0 &&
              g_AssetRequestType == ASSET_REQUEST_IDLE,
          "asset reset clears every loader state");
    Check(EnableCdAudioMode() == 1, "native CD audio mode is synchronous");
    Check(RequestBootAssets() == 7 && s_requestCalls == 1,
          "boot request uses the shared request handshake");

    if (s_failures != 0) return 1;
    puts("native asset loading normalizes failures and guards boot upload");
    return 0;
}
