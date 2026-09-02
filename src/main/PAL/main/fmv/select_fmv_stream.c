#include "game/asset.h"
#include "game/fmv.h"
#include "game/race.h"

enum {
    INTRO_FMV_STREAM = 0,
    GRAND_PRIX_FMV_STREAM_BASE = 1,
    EXTRA_GRAND_PRIX_FMV_STREAM_BASE = 5,
    ENDING_FMV_STREAM = 10,
    GRAND_PRIX_CLASS_COUNT = 4,
    STANDARD_FMV_SECTOR_LIMIT_MULTIPLIER = 2,
    ENDING_FMV_SECTOR_LIMIT_MULTIPLIER = 4,
};

static s32 ClampGrandPrixClass(s32 classIndex) {
    if (classIndex < 0) {
        return 0;
    }
    return classIndex >= GRAND_PRIX_CLASS_COUNT
               ? GRAND_PRIX_CLASS_COUNT - 1
               : classIndex;
}

static void SelectFmvStream(s32 index, u32 sectorLimitMultiplier) {
    GameCdLoadEntry *stream = &g_StreamCdEntries[index];

    g_StreamLoc = stream;
    g_StreamSectorCount = stream->size;
    g_StreamSectorLimit = stream->size * sectorLimitMultiplier;
}

void BeginIntroFmv(s32 returnScene) {
    BeginFmv(returnScene);
    SelectFmvStream(INTRO_FMV_STREAM, STANDARD_FMV_SECTOR_LIMIT_MULTIPLIER);
}

void BeginClassFmv(s32 returnScene) {
    s32 base = g_SeriesSelection == 0
                   ? GRAND_PRIX_FMV_STREAM_BASE
                   : EXTRA_GRAND_PRIX_FMV_STREAM_BASE;

    BeginFmv(returnScene);
    SelectFmvStream(base + ClampGrandPrixClass(g_GrandPrixClass),
                    STANDARD_FMV_SECTOR_LIMIT_MULTIPLIER);
}

void BeginEndingFmv(s32 returnScene) {
    BeginFmv(returnScene);
    SelectFmvStream(ENDING_FMV_STREAM,
                    ENDING_FMV_SECTOR_LIMIT_MULTIPLIER);
}
