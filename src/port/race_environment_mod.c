#include "race_mods.h"
#include "host_disc.h"
#include "content/track_pack.h"
#include "game/asset.h"
#include "game/asset_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/track.h"
#include "game/track_internal.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MAX_ENVIRONMENT_ASSET = 16 * 1024 * 1024, ENVIRONMENT_PROFILES = 5 };
static void *s_profile;
static const GameEnvironmentCue *s_activeCues;
static const GameEnvironmentCue *s_originalCues;
static const EnvironmentPalette *s_originalPalette;
static s32 s_originalLength, s_originalSkyRow;

enum { SKY_X = 512, SKY_Y = 256, SKY_W = 64, SKY_H = 256 };
static u16 s_originalSky[SKY_W * SKY_H];
static int s_haveOriginalSky;
static uint32_t s_skyAssetKey;

uint32_t PortRaceEnvironmentSkyAssetKey(uint32_t original) {
    return s_activeCues && g_EnvScriptCues == s_activeCues && s_skyAssetKey
        ? s_skyAssetKey : original;
}

static int ReadSkyPixels(int profile, u16 *sky) {
    int asset = TrackCourseAssetIndex(ASSET_TRACK_1ST_BASE, profile, g_CourseIndex);
    unsigned size = g_AssetCdEntries[asset].size;
    s32 offsets[5];
    u8 *data;
    int valid = 0;
    if (size < sizeof(offsets) || size > MAX_ENVIRONMENT_ASSET) return 0;
    data = malloc(size);
    if (!data) return 0;
    if (HostLoadAsset(g_AssetCdEntries[asset].position.sectorOffset, size, data) !=
        (int)(size & ~3u)) { free(data); return 0; }
    memcpy(offsets, data, sizeof(offsets));
    /* The third texture-pack block is the original page-24 cloud atlas. */
    if (offsets[2] >= (s32)sizeof(offsets) && offsets[3] > offsets[2] &&
        (unsigned)offsets[3] <= size) {
        const GameImageEntryHeader *entry = (const void *)(data + offsets[2]);
        if (IsValidImageEntry(entry, (size_t)(offsets[3] - offsets[2]))) {
            const GameImageBlock *pixels = (const void *)(entry + 1);
            if (entry->flags & GAME_IMAGE_ENTRY_HAS_CLUT)
                pixels = (const void *)((const u8 *)pixels + pixels->size);
            if (pixels->x == SKY_X && pixels->y == SKY_Y &&
                pixels->w == SKY_W && pixels->h == SKY_H) {
                memcpy(sky, (const u8 *)pixels + offsetof(GameImageBlock, pixels),
                       SKY_W * SKY_H * sizeof(*sky));
                valid = 1;
            }
        }
    }
    free(data);
    return valid;
}

int PortApplyRaceEnvironment(int originalOffset, int grandPrix) {
    TrackBlock blocks[TRACK_PACK_BLOCK_COUNT];
    int profile, asset, offset;
    unsigned size;
    void *data;
    u16 sky[SKY_W * SKY_H];
    Rect skyRect = {SKY_X, SKY_Y, SKY_W, SKY_H};

    /* An asset load installs a fresh original script. A retry can instead
     * retain our borrowed profile, so restore its original before replacing
     * or disabling it. Never release a buffer still referenced by the game. */
    if (s_activeCues == NULL || g_EnvScriptCues != s_activeCues) {
        s_haveOriginalSky = 0;
        s_originalCues = g_EnvScriptCues;
        s_originalPalette = g_EnvPaletteTable;
        s_originalLength = g_EnvScriptLength;
        s_originalSkyRow = g_SkyRowBase;
    } else {
        g_EnvScriptCues = s_originalCues;
        g_EnvPaletteTable = s_originalPalette;
        g_EnvScriptLength = s_originalLength;
        g_SkyRowBase = s_originalSkyRow;
        if (s_haveOriginalSky) { LoadImage(&skyRect, (u_long *)s_originalSky); DrawSync(0); }
    }
    s_activeCues = NULL;
    s_skyAssetKey = 0;
    free(s_profile);
    s_profile = NULL;
    if (!grandPrix || (unsigned)g_CourseIndex >= 4u ||
        !PortRandomTimeOfDayEnabled()) return originalOffset;
    int minimum = g_CourseIndex == 3 ? 2 : 0;
    profile = minimum + PortRaceEnvironmentProfile(
        g_GrandPrixClass - minimum, 1, ENVIRONMENT_PROFILES - minimum);
    if (profile == g_GrandPrixClass) return originalOffset;
    asset = TrackCourseAssetIndex(ASSET_TRACK_2ND_BASE, profile, g_CourseIndex);
    if (asset < 0 || asset >= GAME_DISC_ASSET_COUNT) return originalOffset;
    size = g_AssetCdEntries[asset].size;
    if (size == 0 || size > MAX_ENVIRONMENT_ASSET) return originalOffset;
    data = malloc(size);
    if (data == NULL) return originalOffset;
    if (HostLoadAsset(g_AssetCdEntries[asset].position.sectorOffset, size, data) !=
            (int)(size & ~3u) || !TrackPackOpen(data, size, blocks) ||
        blocks[0].size < offsetof(TrackRenderTable, environmentScriptOffset) + sizeof(s32) ||
        blocks[1].size < ENVIRONMENT_PALETTE_COUNT * sizeof(EnvironmentPalette) ||
        !IsValidEnvironmentScript((const GameEnvironmentScript *)blocks[2].data, blocks[2].size)) {
        fprintf(stderr, "rage-port: could not load time-of-day profile asset=%d; keeping original\n", asset);
        free(data);
        return originalOffset;
    }
    StoreImage(&skyRect, (u_long *)sky); DrawSync(0);
    if (!ReadSkyPixels(profile, sky)) {
        fprintf(stderr, "rage-port: could not load time-of-day sky; keeping original\n");
        free(data);
        return originalOffset;
    }
    StoreImage(&skyRect, (u_long *)s_originalSky); DrawSync(0);
    s_haveOriginalSky = 1;
    LoadImage(&skyRect, (u_long *)sky); DrawSync(0);
    memcpy(&offset, blocks[0].data + offsetof(TrackRenderTable, environmentScriptOffset), sizeof(offset));
    SetEnvironmentScript((const GameEnvironmentScript *)blocks[2].data, blocks[2].size);
    g_EnvPaletteTable = (const EnvironmentPalette *)blocks[1].data;
    s_profile = data;
    s_activeCues = g_EnvScriptCues;
    s_skyAssetKey = (uint32_t)asset;
    fprintf(stderr, "rage-port: random GP time-of-day profile=%d asset=%d offset=%d race_class=%d\n",
            profile, asset, offset, g_GrandPrixClass);
    return offset;
}
