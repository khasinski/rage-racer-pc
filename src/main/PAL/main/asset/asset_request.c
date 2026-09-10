#include "game/asset_internal.h"
#include "game/cd.h"

#include <string.h>

static AssetLoadTransaction s_transaction;

static AssetLoadSpan Span(u8 *data, size_t size) {
    AssetLoadSpan span;

    span.data = data;
    span.size = data != NULL ? size : 0;
    return span;
}

static void CompleteTransaction(void) {
    size_t primarySize = 0;

    if (s_transaction.complete || s_transaction.failed) return;
    s_transaction.complete = 1;
    s_transaction.primary.data = g_AssetBase;
    if (!AssetSpanSize(g_AssetBase, g_ImageBlockBuffer, &primarySize)) {
        /* Not every request carries an image boundary.  Its ordinary output
         * remains available through the typed block spans below. */
        primarySize = 0;
    }
    s_transaction.primary.size = primarySize;
    s_transaction.image = Span(g_ImageBlockBuffer, g_ImageBlockSize);
    s_transaction.block = Span(g_AssetBlockPtr, g_AssetBlockSize);
    s_transaction.block2 = Span(g_AssetBlockPtr2, g_AssetBlock2Size);
    s_transaction.auxiliary = Span(g_AssetSubBlockPtr, g_AssetSubBlockSize);
    switch (s_transaction.request) {
    case ASSET_REQUEST_SELECT_BGM:
        s_transaction.payload.selectBgm.texturePack = s_transaction.primary;
        s_transaction.payload.selectBgm.audioHeader = s_transaction.block;
        s_transaction.payload.selectBgm.sequence = s_transaction.block2;
        s_transaction.payload.selectBgm.audioBody = s_transaction.auxiliary;
        break;
    case ASSET_REQUEST_OPTION_SCREEN:
    case ASSET_REQUEST_SAVE_SCREEN:
    case ASSET_REQUEST_CAR_SELECT:
        s_transaction.payload.screen.image = s_transaction.image;
        break;
    case ASSET_REQUEST_RACE:
    case ASSET_REQUEST_COURSE_TEXTURES:
    case ASSET_REQUEST_TRACK_DATA:
        s_transaction.payload.race.texturePack = s_transaction.primary;
        s_transaction.payload.race.runtime = s_transaction.block;
        s_transaction.payload.race.audio = s_transaction.auxiliary;
        break;
    case ASSET_REQUEST_ROUND_SCREEN:
        s_transaction.payload.round.image = s_transaction.image;
        s_transaction.payload.round.voiceHeader = s_transaction.block2;
        s_transaction.payload.round.voiceBody = s_transaction.auxiliary;
        break;
    default:
        break;
    }
}

const AssetLoadTransaction *AssetLoadTransactionResult(
    AssetRequestType request, u32 generation) {
    if (s_transaction.request != request ||
        s_transaction.generation != generation || !s_transaction.complete ||
        s_transaction.failed) {
        return NULL;
    }
    return &s_transaction;
}

const AssetLoadTransaction *AssetLoadTransactionCurrentResult(u32 generation) {
    if (s_transaction.generation != generation || !s_transaction.complete ||
        s_transaction.failed) {
        return NULL;
    }
    return &s_transaction;
}

u32 AssetLoadTransactionGeneration(void) {
    return s_transaction.generation;
}

void ResetAssetLoadTransaction(void) {
    u32 generation = s_transaction.generation + 1;

    if (generation == 0) generation = 1;
    memset(&s_transaction, 0, sizeof(s_transaction));
    s_transaction.request = ASSET_REQUEST_IDLE;
    s_transaction.generation = generation;
}

s32 AssetLoadHasFailed(void) {
    return g_AssetLoadFailed != 0;
}

s32 AssetLoadCompletedSuccessfully(void) {
    if (g_AssetLoadState != 0 || g_AssetLoadFailed != 0) return 0;
    CompleteTransaction();
    return 1;
}

static s32 StartAssetLoad(AssetRequestType request, s32 firstLoadState,
                          s32 resetCdAudio) {
    if (resetCdAudio) {
        ResetCdAudioState();
    }
    ResetAssetLoadTransaction();
    s_transaction.request = request;
    g_AssetRequestType = request;
    g_AssetLoadFailed = 0;
    g_AssetLoadState = firstLoadState;
    return 1;
}

s32 RequestAssetLoad(AssetRequestType request, s32 firstLoadState,
                     s32 resetCdAudio) {
    if (g_AssetLoadState != 0) {
        return 1;
    }

    if (g_AssetRequestType == request) {
        g_AssetRequestType = ASSET_REQUEST_IDLE;
        return g_AssetLoadFailed != 0 ? -1 : 0;
    }

    return StartAssetLoad(request, firstLoadState, resetCdAudio);
}

s32 RestartAssetLoad(AssetRequestType request, s32 firstLoadState,
                     s32 resetCdAudio) {
    if (g_AssetLoadState != 0) {
        return 0;
    }
    return StartAssetLoad(request, firstLoadState, resetCdAudio);
}
