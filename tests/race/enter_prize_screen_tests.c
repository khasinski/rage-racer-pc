#include "common.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/state.h"

#include <stdio.h>
#include <string.h>

s32 g_ClassPromoted;
s32 g_ClassResultPlace;
s32 g_ClassCompleted;
s32 g_CourseIndex;
s32 g_FrameSyncThreshold;
s32 g_GrandPrixClass;
s32 g_PrizeAmount;
s32 g_PromotionBonus;
s32 g_PromotionBonusTable[PROMOTION_BONUS_COUNT];
s32 g_SceneId;
s32 g_SceneTimer;
s32 g_SeriesCleared;
GameRaceProgress *g_RaceProgress;
PlayerCarRuntime g_PlayerCar;
RagePrizeMoneyStorage g_PrizeMoneyState;
u16 g_PadHeld;
u16 g_PadPressed;

static GameRaceProgress s_progress;
static PrizeScreen s_screen;
static s32 s_failures;
static s32 s_fanfareStarts;


void StartClassClearFanfare(void) {
    s_fanfareStarts++;
}

s32 TickClassClearFanfare(void) { return 0; }
void AdvanceGrandPrixClass(void) {}
void DrawFullscreenFadeTile(s32 step, s32 clut) { (void)step; (void)clut; }
void DrawGrandPrixIntro(s32 drawClassBanner) { (void)drawClassBanner; }
void DrawPrizeMoneyPanel(s32 step) { (void)step; }
void DrawRaceTimePanel(s32 step) { (void)step; }
void PlaySoundCue(s32 cue) { (void)cue; }
s32 RequestSelectBgmAssets(void) { return 0; }

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
    g_ClassResultPlace = 0;
    g_GrandPrixClass = 0;
    g_CourseIndex = 0;
    s_fanfareStarts = 0;
}

int main(void) {
    for (s32 classIndex = 0; classIndex < 6; ++classIndex) {
        Reset();
        g_GrandPrixClass = classIndex;
        g_ClassPromoted = 1;
        g_PlayerCar.drive.racePosition = 1;
        for (s32 i = 0; i < 6; ++i)
            g_PrizeMoney.values[0][i][0] = 1000 + i;
        for (s32 i = 0; i < PROMOTION_BONUS_COUNT; ++i)
            g_PromotionBonusTable[i] = 2000 + i;
        EnterPrizeScreenState(&s_screen);
        Check("every class reward mapping", g_PrizeAmount, 1000 + classIndex);
        Check("every class promotion mapping", g_PromotionBonus,
              classIndex < 5 ? 2000 + classIndex : 0);
    }
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

    EnterPrizeScreenState(&s_screen);
    Check("prize screen scene", g_SceneId, 0x13);
    Check("initial fade timer", g_SceneTimer, 0x100);
    Check("frame sync threshold", g_FrameSyncThreshold, 0x80);
    Check("second-place prize", g_PrizeAmount, 16000);
    Check("promotion bonus", g_PromotionBonus, 100000);
    Check("prize count step", s_screen.prizeStep, 100);
    Check("bonus count step", s_screen.bonusStep, 400);
    Check("loaded money is clamped", s_progress.money,
          RACE_MAX_PRIZE_MONEY);
    Check("ordinary result has no class fanfare", s_fanfareStarts, 0);

    Reset();
    g_ClassResultPlace = 1;
    EnterPrizeScreenState(&s_screen);
    Check("class result starts fanfare", s_fanfareStarts, 1);

    Reset();
    g_CourseIndex = 5;
    g_GrandPrixClass = 4;
    g_PlayerCar.drive.racePosition = 1;
    g_PrizeMoney.values[1][4][PRIZE_PLACE_FIRST] = 54321;
    g_PrizeMoney.values[1][4][PRIZE_PLACE_THIRD] = 160;
    EnterPrizeScreenState(&s_screen);
    Check("Extra GP uses course within series", g_PrizeAmount, 54321);
    Check("Extra GP count step", s_screen.prizeStep, 2);

    Reset();
    g_GrandPrixClass = GRAND_PRIX_FINAL_CLASS_INDEX;
    g_ClassPromoted = 1;
    g_PlayerCar.drive.racePosition = 4;
    EnterPrizeScreenState(&s_screen);
    Check("place outside prize table earns zero", g_PrizeAmount, 0);
    Check("shared finale has no promotion bonus", g_PromotionBonus, 0);
    Check("zero bonus still has a progressing step", s_screen.bonusStep, 1);

    Reset();
    g_GrandPrixClass = -1;
    g_ClassPromoted = 1;
    g_PlayerCar.drive.racePosition = 1;
    EnterPrizeScreenState(&s_screen);
    Check("negative class earns no prize", g_PrizeAmount, 0);
    Check("negative class earns no promotion bonus", g_PromotionBonus, 0);

    Reset();
    g_GrandPrixClass = GRAND_PRIX_PRIZE_CLASS_COUNT;
    g_PlayerCar.drive.racePosition = 1;
    EnterPrizeScreenState(&s_screen);
    Check("class past prize table earns no prize", g_PrizeAmount, 0);

    Reset();
    g_RaceProgress = NULL;
    g_PlayerCar.drive.racePosition = 1;
    g_PrizeMoney.values[0][0][PRIZE_PLACE_FIRST] = 1234;
    EnterPrizeScreenState(&s_screen);
    Check("missing progress still prepares the scene", g_SceneId, 0x13);
    Check("missing progress cannot start a payout", g_PrizeAmount, 0);

    return s_failures != 0;
}
