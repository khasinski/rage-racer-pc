#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/state.h"

enum {
    PRIZE_SCREEN_SCENE = 0x13,
    PRIZE_SCREEN_INITIAL_TIMER = 0x100,
    PRIZE_SCREEN_FRAME_SYNC_THRESHOLD = 0x80,
    PRIZE_COUNT_FRAMES = 80,
    BONUS_COUNT_FRAMES = 250,
};

void EnterPrizeScreen(void) {
    s32 courseIndex;
    s32 classIndex;
    const s32 *prizes;

    g_SceneTimer = PRIZE_SCREEN_INITIAL_TIMER;
    g_FrameSyncThreshold = PRIZE_SCREEN_FRAME_SYNC_THRESHOLD;
    g_RaceProgress->money.value = ClampPrizeMoney(g_RaceProgress->money.value);

    courseIndex = SeriesCourseIndex();
    classIndex = g_GrandPrixClass;
    prizes = g_PrizeMoney.values[courseIndex][classIndex];
    g_PrizeScreenState = PRIZE_SCREEN_STATE_INTRO_FADE_IN;
    g_PrizeAmount = PrizeForRacePosition(
        prizes, PRIZE_PLACE_COUNT, g_PlayerCar.drive.racePosition);
    g_SceneId = PRIZE_SCREEN_SCENE;

    g_PromotionBonus = PromotionBonusForClass(
        g_PromotionBonusTable, PROMOTION_BONUS_COUNT, classIndex,
        g_ClassPromoted);
    g_PrizeCountStep = PrizeCountStep(
        prizes[PRIZE_PLACE_THIRD], PRIZE_COUNT_FRAMES);
    g_BonusCountStep = PrizeCountStep(
        g_PromotionBonus, BONUS_COUNT_FRAMES);
}
