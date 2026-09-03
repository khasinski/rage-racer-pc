#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 g_BonusCountStep;
s32 g_ClassPromoted;
s32 g_CourseIndex;
s32 g_FrameSyncThreshold;
s32 g_GrandPrixClass;
s32 g_PrizeAmount;
s32 g_PrizeCountStep;
s32 g_PromotionBonus;
s32 g_PromotionBonusTable[PROMOTION_BONUS_COUNT];
s32 g_SceneId;
s32 g_SceneTimer;
PrizeScreenState g_PrizeScreenState;
GameRaceProgress *g_RaceProgress;
PlayerCarRuntime g_PlayerCar;
RagePrizeMoneyStorage g_PrizeMoneyState;

static GameRaceProgress s_progress;
static s32 s_failures;

static void Check(const char *name, s32 actual, s32 expected) {
    if (actual != expected) {
        printf("FAIL %s: got %d, expected %d\n", name, actual, expected);
        s_failures++;
    }
}

static void Reset(void) {
    memset(&s_progress, 0, sizeof(s_progress));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    memset(&g_PrizeMoneyState, 0, sizeof(g_PrizeMoneyState));
    memset(g_PromotionBonusTable, 0, sizeof(g_PromotionBonusTable));
    g_RaceProgress = &s_progress;
    g_ClassPromoted = 0;
    g_GrandPrixClass = 0;
    g_CourseIndex = 0;
}

int main(void) {
    Reset();
    g_CourseIndex = 2;
    g_GrandPrixClass = 3;
    g_ClassPromoted = 1;
    g_PlayerCar.drive.racePosition = 2;
    g_PrizeMoney.values[2][3][PRIZE_PLACE_FIRST] = 30000;
    g_PrizeMoney.values[2][3][PRIZE_PLACE_SECOND] = 16000;
    g_PrizeMoney.values[2][3][PRIZE_PLACE_THIRD] = 8000;
    g_PromotionBonusTable[3] = 100000;
    s_progress.money = 1000000000;

    EnterPrizeScreen();
    Check("prize screen scene", g_SceneId, 0x13);
    Check("initial fade timer", g_SceneTimer, 0x100);
    Check("frame sync threshold", g_FrameSyncThreshold, 0x80);
    Check("initial prize screen state", g_PrizeScreenState,
          PRIZE_SCREEN_STATE_INTRO_FADE_IN);
    Check("second-place prize", g_PrizeAmount, 16000);
    Check("promotion bonus", g_PromotionBonus, 100000);
    Check("prize count step", g_PrizeCountStep, 100);
    Check("bonus count step", g_BonusCountStep, 400);
    Check("loaded money is clamped", s_progress.money,
          RACE_MAX_PRIZE_MONEY);

    Reset();
    g_CourseIndex = 5;
    g_GrandPrixClass = 4;
    g_PlayerCar.drive.racePosition = 1;
    g_PrizeMoney.values[1][4][PRIZE_PLACE_FIRST] = 54321;
    g_PrizeMoney.values[1][4][PRIZE_PLACE_THIRD] = 160;
    EnterPrizeScreen();
    Check("Extra GP uses course within series", g_PrizeAmount, 54321);
    Check("Extra GP count step", g_PrizeCountStep, 2);

    Reset();
    g_GrandPrixClass = GRAND_PRIX_FINAL_CLASS_INDEX;
    g_ClassPromoted = 1;
    g_PlayerCar.drive.racePosition = 4;
    EnterPrizeScreen();
    Check("place outside prize table earns zero", g_PrizeAmount, 0);
    Check("shared finale has no promotion bonus", g_PromotionBonus, 0);
    Check("zero bonus still has a progressing step", g_BonusCountStep, 1);

    return s_failures != 0;
}
