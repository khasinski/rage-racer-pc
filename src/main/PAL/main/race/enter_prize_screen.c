#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/scene.h"
#include "game/state.h"

enum {
    PRIZE_SCREEN_INITIAL_TIMER = 0x100,
    PRIZE_SCREEN_FRAME_SYNC_THRESHOLD = 0x80,
    PRIZE_COUNT_FRAMES = 80,
    BONUS_COUNT_FRAMES = 250,
};

void EnterPrizeScreen(void) {
    static const s32 noPrizes[PRIZE_PLACE_COUNT] = {0};
    s32 courseIndex;
    s32 classIndex;
    const s32 *prizes = noPrizes;

    g_SceneTimer = PRIZE_SCREEN_INITIAL_TIMER;
    g_FrameSyncThreshold = PRIZE_SCREEN_FRAME_SYNC_THRESHOLD;
    if (g_RaceProgress != NULL) {
        g_RaceProgress->money = ClampPrizeMoney(g_RaceProgress->money);
    }

    courseIndex = SeriesCourseIndex();
    classIndex = g_GrandPrixClass;
    if (g_RaceProgress != NULL &&
        (u32)classIndex < GRAND_PRIX_PRIZE_CLASS_COUNT) {
        prizes = g_PrizeMoney.values[courseIndex][classIndex];
    }
    g_PrizeScreenState = PRIZE_SCREEN_STATE_INTRO_FADE_IN;
    g_PrizeAmount = PrizeForRacePosition(
        prizes, PRIZE_PLACE_COUNT, g_PlayerCar.drive.racePosition);
    g_SceneId = GAME_SCENE_PRIZE;

    g_PromotionBonus = PromotionBonusForClass(
        g_PromotionBonusTable, PROMOTION_BONUS_COUNT, classIndex,
        g_ClassPromoted);
    g_PrizeCountStep = PrizeCountStep(
        prizes[PRIZE_PLACE_THIRD], PRIZE_COUNT_FRAMES);
    g_BonusCountStep = PrizeCountStep(
        g_PromotionBonus, BONUS_COUNT_FRAMES);
}
