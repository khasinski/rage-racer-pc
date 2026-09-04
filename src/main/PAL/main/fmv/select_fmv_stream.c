#include "game/asset.h"
#include "game/fmv.h"
#include "game/race.h"

static s32 ClampGrandPrixClass(s32 classIndex) {
    if (classIndex < 0) {
        return 0;
    }
    return classIndex >= FMV_GRAND_PRIX_CLASS_COUNT
               ? FMV_GRAND_PRIX_CLASS_COUNT - 1
               : classIndex;
}

static void SelectFmvStream(s32 index) {
    GameCdLoadEntry *stream = &g_StreamCdEntries[index];

    g_StreamLoc = stream;
    g_StreamFrameCount = stream->size;
}

void BeginIntroFmv(s32 returnScene) {
    BeginFmv(returnScene);
    SelectFmvStream(FMV_STREAM_INTRO);
}

void BeginClassFmv(s32 returnScene) {
    s32 base = g_SeriesSelection == 0 ? FMV_STREAM_GRAND_PRIX_BASE
                                      : FMV_STREAM_EXTRA_GRAND_PRIX_BASE;

    BeginFmv(returnScene);
    SelectFmvStream(base + ClampGrandPrixClass(g_GrandPrixClass));
}

void BeginEndingFmv(s32 returnScene) {
    BeginFmv(returnScene);
    SelectFmvStream(FMV_STREAM_ENDING);
}
