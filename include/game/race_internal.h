#ifndef GAME_RACE_INTERNAL_H
#define GAME_RACE_INTERNAL_H

#include "common.h"
#include "game/menu_types.h"

typedef struct PrologueCameraCut {
    s16 timer;
    s16 carIndex;
} PrologueCameraCut;

typedef struct ResultPlaceBarPosition {
    u8 left;
    u8 right;
} ResultPlaceBarPosition;

typedef struct ResultPlaceSpriteLayout {
    u8 x;
    u8 y;
    u8 width;
} ResultPlaceSpriteLayout;

static inline s32 RaceRecordMode(s32 grandPrixMode) {
    return grandPrixMode != 0;
}

s32 ReplayEndingWashActive(s32 sceneTimer, s32 frameCount);
s32 ReplayEndingWashLevel(s32 sceneTimer, s32 frameCount);
s32 ShouldStartReplayExitFade(s32 sceneTimer, s32 frameCount);

typedef union SectorReferenceTimes {
    s32 values[3];
    struct {
        s32 first;
        s32 second;
        s32 third;
    } fields;
} SectorReferenceTimes;

enum { GRAND_PRIX_SHARED_FINAL_CLASS = 5 };

extern s32 g_RaceTotalTime;
extern SectorReferenceTimes g_RefSectorTimes;

/* Holds the cue that follows FINISHED until the special voices are free, so a
 * fast host frame cannot make the second cue replace the first. Lives in the
 * race scene; the lap update is what asks for it. */
void QueueFinishFollowupCue(s32 cue);
extern s32 g_PrologueStep;
extern PrologueCameraCut g_PrologueCameraCuts[];
s32 PrologueLineIntensity(s32 screenY);
s32 IsPrologueWorldActive(s32 sceneTimer);
extern ResultPlaceSpriteLayout g_ResultPlaceSprites[];
extern ResultPlaceBarPosition g_ClassPlaceBarSizes[];

s32 GrandPrixCourseCount(s32 classIndex);
s32 NextUnlockedClassRecord(s32 classRecordIndex);
s32 IsFinalGrandPrixClass(s32 extraSeries, s32 classIndex);
static inline s32 GrandPrixAssetSeries(s32 selectedSeries, s32 classIndex) {
    return classIndex < GRAND_PRIX_SHARED_FINAL_CLASS ? selectedSeries : 0;
}
s32 PrizeCountStep(s32 amount, s32 frameCount);
s32 PromotionBonusForClass(const s32 *bonuses, s32 bonusCount,
                           s32 classIndex, s32 promoted);
s32 PrizeForRacePosition(const s32 *prizes, s32 prizeCount,
                         s32 racePosition);
s32 CountClassWins(const ScoreRecord *records, s32 recordCount);
s32 BgmTrackCountForClassWins(s32 classWinCount);
s32 BestRacePlace(s32 previousPlace, s32 racePosition);
s32 GrandPrixClassIsComplete(const u8 *bestPlaces, s32 courseCount);
s32 BestClassGrade(s32 previousGrade, s32 grade);
u16 UpdatedClassClearCount(u16 clears, s32 grade);
s32 ComputeClassGradeForPlaces(const u8 bestPlaces[4], s32 unlockPending);
s32 RaceEndBrightness(s32 level);
s32 UpdateLostRaceChoice(s32 choice, u16 pressedButtons);
s32 LostRaceExitScene(s32 choice);
s32 CanSkipRaceEndScreen(s32 timer, u16 pressedButtons);
s32 ResultCourseNameY(s32 grandPrixMode);
s32 IsValidRaceResultPlace(s32 racePosition);
s32 ShouldDrawClassPlaceBanner(s32 classPlace, s32 prizeScreenState);
s32 GrandPrixNameIndex(s32 extraSeries, s32 classIndex);

#endif
