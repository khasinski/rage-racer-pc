#include "game/asset.h"
#include "game/menu.h"
#include "game/race.h"
#include "game/audio.h"
#include "game/screens.h"
#include "game/sound.h"

enum {
    PRIZE_SCREEN_FADE_LIMIT = 0x100,
    PRIZE_SCREEN_FADE_CLUT = 0x49,
    PANEL_SLIDE_STEP = 8,
    PANEL_OFFSCREEN_OFFSET = 128,
    PRIZE_COUNT_DELAY_FRAMES = 120,
    FAST_COUNT_MULTIPLIER = 4,
    SOUND_CUE_COUNT = 0xF,
    SOUND_CUE_FAST_COUNT = 0x10,
    SOUND_CUE_CONFIRM = 0x11,
};

/*
 * Move up to `step` of what is still owed into the player's money.
 *
 * The prize and the promotion bonus are counted in the same way, one from
 * PRIZE_SCREEN_STATE_COUNT_PRIZE and the other from COUNT_BONUS, and holding
 * the confirm button counts four times as fast.
 */
static void CountTowardsMoney(s32 *owed, s32 step) {
    s32 amount = *owed;
    s32 payment = amount < step ? amount : step;

    *owed = amount - payment;
    g_RaceProgress->money.value =
        CreditPrizeMoney(g_RaceProgress->money.value, payment);
}

/* Scene 19: counts the prize money and then the class-clear bonus into the save block. */
void UpdatePrizeMoneyScreen(void) {
    s32 prizeStep = g_PrizeCountStep;
    s32 bonusStep = g_BonusCountStep;

    if (g_PadHeld & PAD_CONFIRM) {
        prizeStep *= FAST_COUNT_MULTIPLIER;
        bonusStep *= FAST_COUNT_MULTIPLIER;
    }

    switch (g_PrizeScreenState) {
    case PRIZE_SCREEN_STATE_INTRO_FADE_IN:
        g_SceneTimer -= PANEL_SLIDE_STEP;
        DrawFullscreenFadeTile(g_SceneTimer, PRIZE_SCREEN_FADE_CLUT);
        if (g_SceneTimer == 0) {
            g_PrizeScreenState = PRIZE_SCREEN_STATE_WAIT_FOR_INTRO_CONFIRM;
        }
        DrawRaceTimePanel(0);
        DrawGrandPrixIntro();
        return;
    case PRIZE_SCREEN_STATE_WAIT_FOR_INTRO_CONFIRM:
        DrawRaceTimePanel(0);
        if (g_PadPressed & PAD_CONFIRM) {
            g_PrizeScreenState = PRIZE_SCREEN_STATE_HIDE_RACE_TIME;
            g_SceneTimer = 0;
        }
        DrawGrandPrixIntro();
        return;
    case PRIZE_SCREEN_STATE_HIDE_RACE_TIME:
        g_SceneTimer += PANEL_SLIDE_STEP;
        DrawRaceTimePanel(g_SceneTimer);
        if (g_SceneTimer > PANEL_OFFSCREEN_OFFSET) {
            g_PrizeScreenState = PRIZE_SCREEN_STATE_SHOW_PRIZE_PANEL;
        }
        DrawGrandPrixIntro();
        return;
    case PRIZE_SCREEN_STATE_SHOW_PRIZE_PANEL:
        g_SceneTimer -= PANEL_SLIDE_STEP;
        DrawPrizeMoneyPanel(g_SceneTimer);
        if (g_SceneTimer == 0) {
            g_PrizeScreenState = PRIZE_SCREEN_STATE_COUNT_PRIZE;
        }
        DrawGrandPrixIntro();
        return;
    case PRIZE_SCREEN_STATE_COUNT_PRIZE:
        g_SceneTimer += 1;
        /* The panel settles for two seconds before the counter starts. */
        if (g_SceneTimer > PRIZE_COUNT_DELAY_FRAMES && g_PrizeAmount != 0) {
            PlaySoundCue((g_PadHeld & PAD_CONFIRM)
                             ? SOUND_CUE_FAST_COUNT
                             : SOUND_CUE_COUNT);
            CountTowardsMoney(&g_PrizeAmount, prizeStep);
        }
        if (g_PrizeAmount != 0) {
            break;
        }
        g_SceneTimer = 0;
        g_PrizeScreenState = g_PromotionBonus == 0
                                 ? PRIZE_SCREEN_STATE_WAIT_TO_FINISH
                                 : PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM;
        break;
    case PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM:
        PlaySoundCue(SOUND_CUE_CONFIRM);
        if (g_PadPressed & PAD_CONFIRM) {
            g_PrizeScreenState = PRIZE_SCREEN_STATE_COUNT_BONUS;
        }
        break;
    case PRIZE_SCREEN_STATE_COUNT_BONUS:
        TickClassClearFanfare();
        if (g_PromotionBonus != 0) {
            PlaySoundCue((g_PadHeld & PAD_CONFIRM)
                             ? SOUND_CUE_FAST_COUNT
                             : SOUND_CUE_COUNT);
            CountTowardsMoney(&g_PromotionBonus, bonusStep);
            if (g_PromotionBonus != 0) {
                break;
            }
        }
        g_PrizeScreenState = PRIZE_SCREEN_STATE_WAIT_TO_FINISH;
        break;
    case PRIZE_SCREEN_STATE_WAIT_TO_FINISH:
        TickClassClearFanfare();
        PlaySoundCue(SOUND_CUE_CONFIRM);
        if (!(g_PadPressed & PAD_CONFIRM) || g_ClassClearFanfareTimer != 0) {
            break;
        }
        if (g_ClassCompleted == 0) {
            RequestSelectBgmAssets();
        }
        g_PrizeScreenState = PRIZE_SCREEN_STATE_FADE_OUT;
        break;
    case PRIZE_SCREEN_STATE_FADE_OUT:
        g_SceneTimer += g_SeriesCleared != 0 ? 1 : 2;
        DrawFullscreenFadeTile(g_SceneTimer, PRIZE_SCREEN_FADE_CLUT);
        if (g_SceneTimer >= PRIZE_SCREEN_FADE_LIMIT) {
            AdvanceGrandPrixClass();
        }
        break;
    default:
        DrawGrandPrixIntro();
        return;
    }
    DrawPrizeMoneyPanel(0);
    DrawGrandPrixIntro();
}
