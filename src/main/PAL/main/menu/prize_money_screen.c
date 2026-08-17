#include "common.h"
#include "game/state.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/screens.h"
#include "game/sound.h"
#include "game/car.h"

typedef union PrizeScreenWork {
    s32 value;
    u32 unsignedValue;
} PrizeScreenWork;


/* Scene 19: counts the prize money and then the class-clear bonus into the save block. */
void UpdatePrizeMoneyScreen(void) {
    s32 lim1 = g_PrizeCountStep;
    s32 lim0 = g_BonusCountStep;
    PrizeScreenState st;
    PrizeScreenWork t;

    if (g_PadHeld & PAD_CONFIRM) {
        lim1 <<= 2;
        lim0 <<= 2;
    }

    switch (g_PrizeScreenState) {
    case PRIZE_SCREEN_STATE_INTRO_FADE_IN:
        g_SceneTimer -= 8;
        DrawFullscreenFadeTile(g_SceneTimer, 0x49);
        if (g_SceneTimer == 0) g_PrizeScreenState = PRIZE_SCREEN_STATE_WAIT_FOR_INTRO_CONFIRM;
        DrawRaceTimePanel(0);
        DrawGrandprixIntro();
        return;
    case PRIZE_SCREEN_STATE_WAIT_FOR_INTRO_CONFIRM:
        DrawRaceTimePanel(0);
        if (g_PadPressed & PAD_CONFIRM) {
            g_PrizeScreenState = PRIZE_SCREEN_STATE_HIDE_RACE_TIME;
            g_SceneTimer = 0;
        }
        DrawGrandprixIntro();
        return;
    case PRIZE_SCREEN_STATE_HIDE_RACE_TIME:
        g_SceneTimer += 8;
        DrawRaceTimePanel(g_SceneTimer);
        if (g_SceneTimer >= 129) g_PrizeScreenState = PRIZE_SCREEN_STATE_SHOW_PRIZE_PANEL;
        DrawGrandprixIntro();
        return;
    case PRIZE_SCREEN_STATE_SHOW_PRIZE_PANEL:
        g_SceneTimer -= 8;
        DrawPrizeMoneyPanel(g_SceneTimer);
        if (g_SceneTimer == 0) g_PrizeScreenState = PRIZE_SCREEN_STATE_COUNT_PRIZE;
        DrawGrandprixIntro();
        return;
    case PRIZE_SCREEN_STATE_COUNT_PRIZE:
        g_SceneTimer += 1;
        t.value = g_SceneTimer;
        if (!(t.unsignedValue < 121)) {
        if (g_PrizeAmount == 0) {
        g_SceneTimer = 0;
        if (g_PromotionBonus == 0) goto Lstore7;
        st = PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM;
        goto Lstore;
        }
        PlaySoundCue((g_PadHeld & PAD_CONFIRM) ? 0x10 : 0xf);
        t.value = g_PrizeAmount;
        if (t.value >= lim1) {
            g_PrizeAmount = t.value - lim1;
            g_RaceProgress->money.value += lim1;
        } else {
            s32 e = g_RaceProgress->money.value;
            g_PrizeAmount = 0;
            g_RaceProgress->money.value = e + t.value;
        }
        }
        if (g_PrizeAmount != 0) break;
        g_SceneTimer = 0;
        if (g_PromotionBonus == 0) goto Lstore7;
        st = PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM;
        goto Lstore;
    case PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM:
        PlaySoundCue(0x11);
        if (!(g_PadPressed & PAD_CONFIRM)) break;
        st = PRIZE_SCREEN_STATE_COUNT_BONUS;
    Lstore:
        g_PrizeScreenState = st;
        break;
    case PRIZE_SCREEN_STATE_COUNT_BONUS:
        TickClassClearFanfare();
        if (g_PromotionBonus == 0) { st = PRIZE_SCREEN_STATE_WAIT_TO_FINISH; goto Lstore; }
        PlaySoundCue((g_PadHeld & PAD_CONFIRM) ? 0x10 : 0xf);
        t.value = g_PromotionBonus;
        if (t.value >= lim0) {
            g_PromotionBonus = t.value - lim0;
            g_RaceProgress->money.value += lim0;
        } else {
            s32 e = g_RaceProgress->money.value;
            g_PromotionBonus = 0;
            g_RaceProgress->money.value = e + t.value;
        }
        if (g_PromotionBonus != 0) break;
    Lstore7:
        st = PRIZE_SCREEN_STATE_WAIT_TO_FINISH;
        goto Lstore;
    case PRIZE_SCREEN_STATE_WAIT_TO_FINISH:
        TickClassClearFanfare();
        PlaySoundCue(0x11);
        if (!(g_PadPressed & PAD_CONFIRM)) break;
        if (g_ClassClearFanfareTimer != 0) break;
        if (g_ClassCompleted == 0) { RequestSelectBgmAssets(); }
        st = PRIZE_SCREEN_STATE_FADE_OUT;
        goto Lstore;
    case PRIZE_SCREEN_STATE_FADE_OUT:
        if (g_SeriesCleared != 0)
            g_SceneTimer += 1;
        else
            g_SceneTimer += 2;
        DrawFullscreenFadeTile(g_SceneTimer, 0x49);
        if (g_SceneTimer < 0x100) break;
        AdvanceGrandPrixClass();
        break;
    default:
        DrawGrandprixIntro();
        return;
    }
    DrawPrizeMoneyPanel(0);
    DrawGrandprixIntro();
}
