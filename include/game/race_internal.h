#ifndef GAME_RACE_INTERNAL_H
#define GAME_RACE_INTERNAL_H

#include "common.h"
#include "game/menu_types.h"
#include "game/race.h"

struct PlayerCarRuntime;

enum {
    REPLAY_OPAQUE_FADE = 257,
    RACE_END_SCREEN_FADE_COMPLETE = 256,
    RACE_END_SCREEN_INITIAL_TIMER = 555,
    RACE_FRAMES_PER_SECOND = 25,
};

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

static inline s32 RaceSeriesIndex(s32 series) {
    return (u32)series < 2 ? series : 0;
}

s32 ReplayEndingWashActive(s32 sceneTimer, s32 frameCount);
s32 ReplayEndingWashLevel(s32 sceneTimer, s32 frameCount);
s32 AdvanceReplayFadeLevel(s32 level, s32 step);
s32 ShouldStartReplayExitFade(s32 sceneTimer, s32 frameCount);
s32 ReplayBadgeVisible(s32 sceneTimer, s32 seriesCleared);
s32 ReplayResultCue(s32 sceneTimer, s32 grandPrixMode, s32 seriesCleared,
                    s32 racePosition);
s32 NextReplayReadCursor(s32 cursor, s32 frameCount);
s32 NextReplaySceneTimer(s32 sceneTimer, s32 frameCount);
void PlayCountdownCues(s32 timer);
void UpdateRivalCueGate(void);
void UpdateSplitTimes(struct PlayerCarRuntime *car, s32 grandPrixMode,
                      s32 lapEvent);
void SeedFinishCamera(struct PlayerCarRuntime *car);
void UpdateFinishCamera(struct PlayerCarRuntime *car);
void UpdateRacePosition(void);
s32 AttractTitleFadeLevel(s32 step, s32 timer, s32 fadeLevel, s32 delay);
s32 BgmCdTrack(s32 selectedTrack);
s32 WrapBgmTrackIndex(s32 track, s32 trackCount);
s32 NextBgmSelectTimer(s32 timer);
s32 StepBgmSelectFade(s32 fade, s32 step, s32 ceiling);
s32 AttractOpeningWashLevel(s32 timer);
s32 AttractClosingWashLevel(s32 timer);
s32 NextAttractLoadTimer(s32 timer);
s32 NextAttractRaceTimer(s32 timer);
s32 ShouldStartAttractExitFade(s32 timer);
s32 ShouldReturnFromAttractDemo(s32 timer);
void UpdateAndDrawAttractWorld(void);
void UpdateBgmSelectScene(void);
void UpdateBgmSelectBar(void);
void DrawBgmSelectBar(void);
void AdvanceBgmShuffleBag(u32 track);
void UpdateBgmSelectPlayback(void);
void UpdateBgmSelectInput(void);
void UpdateBgmSelectLoad(void);
void UpdateBgmSelectFadeIn(void);
void UpdateBgmSelect(void);
void ExitBgmSelect(void);
s32 CycleAttractCameraCar(s32 mask, s32 current);
s32 CycleBgmSelectCameraCar(s32 mask, s32 current);
s32 FramesToMilliseconds(s32 frames, s32 subframeMillis);
void CommitClassProgress(void);

typedef union SectorReferenceTimes {
    s32 values[3];
    struct {
        s32 first;
        s32 second;
        s32 third;
    } fields;
} SectorReferenceTimes;

extern s32 g_RaceTotalTime;
extern SectorReferenceTimes g_RefSectorTimes;

/* Holds the cue that follows FINISHED until the special voices are free, so a
 * fast host frame cannot make the second cue replace the first. Lives in the
 * race scene; the lap update is what asks for it. */
void QueueFinishFollowupCue(s32 cue);
extern s32 g_PrologueStep;
enum {
    PROLOGUE_STEP_LOAD_TEXTURES,
    PROLOGUE_STEP_LOAD_TRACK,
    PROLOGUE_STEP_WAIT_FOR_FADE,
    PROLOGUE_STEP_ACTIVE,
    PROLOGUE_END_FRAME = 0x500,
};
extern PrologueCameraCut g_PrologueCameraCuts[PROLOGUE_CAMERA_CUT_COUNT];
s32 PrologueLineIntensity(s32 screenY);
s32 IsPrologueWorldActive(s32 sceneTimer);
s32 NextPrologueTimer(s32 sceneTimer);
s32 AdvancePrologueFade(s32 level, s32 step, s32 maximum);
s32 PrologueCameraIndex(s32 cameraIndex);
s32 PrologueCameraCutIndex(s32 cutIndex);
extern ResultPlaceSpriteLayout g_ResultPlaceSprites[];
extern ResultPlaceBarPosition g_ClassPlaceBarSizes[];

s32 GrandPrixCourseCount(s32 classIndex);
s32 GrandPrixClassRecordIndex(s32 series, s32 classIndex);
s32 NextUnlockedClassRecord(s32 classRecordIndex);
s32 IsFinalGrandPrixClass(s32 extraSeries, s32 classIndex);
s32 NextGrandPrixClassForSeries(s32 series, s32 classIndex);
static inline s32 GrandPrixAssetSeries(s32 selectedSeries, s32 classIndex) {
    return classIndex < GRAND_PRIX_FINAL_CLASS_INDEX ? selectedSeries : 0;
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
s32 LostRaceRetryDigitIndex(s32 retriesRemaining);
s32 LostRaceExitScene(s32 choice);
s32 CanSkipRaceEndScreen(s32 timer, u16 pressedButtons);
s32 NextLostRaceFadeTimer(s32 timer);
s32 NextRaceEndScreenTimer(s32 timer);
s32 ResultCourseNameY(s32 grandPrixMode);
s32 IsValidRaceResultPlace(s32 racePosition);
s32 ShouldDrawClassPlaceBanner(s32 classPlace, s32 prizeScreenState);
s32 GrandPrixNameIndex(s32 extraSeries, s32 classIndex);
s32 GrandPrixClassDisplayNumber(s32 extraSeries, s32 classIndex);

#endif
