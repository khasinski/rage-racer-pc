#include "game/asset.h"
#include "game/fmv.h"
#include "game/race.h"
#include "game/grand_prix_content.h"

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
    const GrandPrixClassDefinition *definition =
        GrandPrixContentClass(ClampGrandPrixClass(g_GrandPrixClass));

    BeginFmv(returnScene);
    SelectFmvStream(definition->promotionStream[g_SeriesSelection != 0]);
}

void BeginEndingFmv(s32 returnScene) {
    BeginFmv(returnScene);
    SelectFmvStream(FMV_STREAM_ENDING);
}
