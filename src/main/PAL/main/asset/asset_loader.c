/*
 * The disc asset loader: open RAGE.BIN, read a numbered asset into the load
 * buffer, and hand out its sub-blocks. SetTrackCameraTable looks like track
 * code from its name but is part of this: it installs sub-block 0 of the
 * loaded .2ND track pack as the TrackRenderTable base, and its only callers
 * are load_track_data_assets.c and race_assets.c. That name is why this unit
 * sat in track/ until 2026-08-03.
 */
#include "common.h"
#include <stdio.h>
#include "game/state.h"
#include "game/asset.h"
#include "game/render_internal.h"
#include "psyq/cd.h"


void SetTrackCameraTable(void *table) {
    g_TrackRenderTable = table;
}

void ResetAssetLoader(void) {
    if (g_CdLoadPhase == 4) {
        CdReadBreak();
    }

    g_CdLoadPhase = 0;
    g_AssetLoadState = 0;
    g_AssetRequestType = ASSET_REQUEST_IDLE;
}

s32 EnableCdAudioMode(void) {
    u8 value;

    if (CdSync(1, 0) == 0) {
        return 0;
    }

    value = 7;
    return CdControl(0xE, &value, 0);
}

s32 LoadAsset(s32 assetIndex, void *dst) {
    s32 result;
    s32 size;

    switch (g_CdLoadPhase) {
    case 0:
        printf(g_MsgNowLoading, g_AssetPaths[assetIndex], dst);
        if (CdSync(1, 0) != 0) {
            g_CdLoadPhase = 1;
        }
        return 0;

    case 1:
        CdControlF(2, &g_AssetCdEntries[assetIndex]);
        g_CdLoadPhase = 2;
        return 0;

    case 2:
        if (CdSync(1, 0) != 0) {
            g_CdLoadPhase = 3;
        }
        return 0;

    case 3:
        if (CdRead((g_AssetCdEntries[assetIndex].size + CD_SECTOR_MASK) >> 11, dst, CdlModeSpeed) != 0) {
            g_CdLoadPhase = 4;
        }
        return 0;

    case 4:
        result = CdReadSync(1, 0);
        if (result == 0) {
            g_CdLoadPhase = 5;
            return 0;
        }
        if (result == -1) {
            g_CdLoadPhase = 6;
        }
        return 0;

    case 5:
        size = (g_AssetCdEntries[assetIndex].size >> 2) << 2;
        printf(g_MsgReadBytes, size);
        g_CdLoadPhase = 0;
        return size;

    case 6:
        printf(g_MsgFileReadError, g_AssetPaths[assetIndex], dst);
        g_CdLoadPhase = 0;
        break;
    }

    return 0;
}

void LoadAssetBlocking(s32 assetIndex, void *dst) {
    while (LoadAsset(assetIndex, dst) == 0) {
    }
}

void LoadDiscArchiveIndex(void) {
    CdlFILE file;
    s32 sectors;
    s32 base;
    s32 i;
    s32 status;
    s32 *src;
    GameCdLoadEntry *dst;
    GameCdLoadEntry *stream;

    printf(g_MsgNowLoading, g_PathRageBin, g_LoadBuffer);
    if (DsSearchFile(&file, g_PathRageBin) == 0) {
        printf(g_MsgFileNotFound, g_PathRageBin);
    }

    sectors = 1;
    do {
        CdControl(2, &file.pos, 0);
        CdRead(sectors, g_LoadBuffer, CdlModeSpeed);
        do {
            status = CdReadSync(1, 0);
        } while (status > 0);
    } while (status != 0);

    printf(g_MsgReadSectors, sectors);
    base = CdPosToInt_Local(&file.pos);
    src = g_LoadBuffer;
    dst = g_AssetCdEntries;
    for (i = 0; i < 135; i++) {
        CdIntToPos(base + *src, &dst->position.location);
        dst->size = src[1];
        src += 2;
        dst++;
    }

    printf(g_MsgNowSearching, g_PathRageStr);
    if (DsSearchFile(&file, g_PathRageStr) == 0) {
        printf(g_MsgFileNotFound, g_PathRageStr);
    } else {
        printf(g_MsgSearchOk);
    }

    base = CdPosToInt_Local(&file.pos);
    stream = g_StreamCdEntries;
    for (i = 0; i < 11; i++) {
        CdIntToPos(base + stream->position.sectorOffset,
                   &stream->position.location);
        stream++;
    }
}

void InitAssetSystem(void) {
    void *ptr;

    LoadDiscArchiveIndex();
    ptr = &g_LoadBuffer;
    LoadAssetBlocking(0, ptr);
    UploadImageAsset(ptr);
}

s32 RequestBootAssets(void) {
    if (g_AssetLoadState != 0) {
        return 1;
    }

    if (g_AssetRequestType == ASSET_REQUEST_BOOT) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return 0;
    }

    g_AssetRequestType = ASSET_REQUEST_BOOT;
    g_AssetLoadState = 1;
    return 1;
}
