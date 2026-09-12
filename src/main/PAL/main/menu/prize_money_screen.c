#include "game/asset.h"
#include "game/menu.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/race_internal.h"
#include "game/audio.h"
#include "game/scene.h"
#include "game/screens.h"
#include "game/sound.h"
#include "game/state.h"

#include <limits.h>
#include <stdint.h>

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
    PRIZE_SCREEN_INITIAL_TIMER = 0x100,
    PRIZE_SCREEN_FRAME_SYNC_THRESHOLD = 0x80,
    PRIZE_COUNT_FRAMES = 80,
    BONUS_COUNT_FRAMES = 250,
};

static PrizeScreen s_screen;

void EnterPrizeScreenState(PrizeScreen *screen) {
    static const s32 noPrizes[PRIZE_PLACE_COUNT] = {0};
    s32 courseIndex = SeriesCourseIndex();
    const GrandPrixClassDefinition *definition =
        GrandPrixContentClass(g_GrandPrixClass);
    const s32 *prizes = noPrizes;

    screen->timer = PRIZE_SCREEN_INITIAL_TIMER;
    g_FrameSyncThreshold = PRIZE_SCREEN_FRAME_SYNC_THRESHOLD;
    if (g_RaceProgress != NULL) {
        g_RaceProgress->money = ClampPrizeMoney(g_RaceProgress->money);
    }
    if (g_RaceProgress != NULL && definition != NULL &&
        (u32)definition->prizeClass < GRAND_PRIX_PRIZE_CLASS_COUNT) {
        prizes = g_PrizeMoney.values[courseIndex][definition->prizeClass];
    }

    screen->state = PRIZE_SCREEN_STATE_INTRO_FADE_IN;
    screen->prize = PrizeForRacePosition(
        prizes, PRIZE_PLACE_COUNT, g_PlayerCar.drive.racePosition);
    screen->bonus = PromotionBonusForClass(
        g_PromotionBonusTable, PROMOTION_BONUS_COUNT,
        definition != NULL ? definition->promotionBonusIndex : -1,
        g_ClassPromoted);
    screen->prizeStep = PrizeCountStep(
        prizes[PRIZE_PLACE_THIRD], PRIZE_COUNT_FRAMES);
    screen->bonusStep = PrizeCountStep(screen->bonus, BONUS_COUNT_FRAMES);
    if (g_ClassResultPlace != 0) {
        StartClassClearFanfare();
    }
    g_SceneId = GAME_SCENE_PRIZE;
}

void EnterPrizeScreen(void) {
    EnterPrizeScreenState(&s_screen);
}

static s32 AddClampedScreenValue(s32 value, s32 delta, s32 maximum) {
    int64_t next = (int64_t)value + delta;

    if (next <= 0) {
        return 0;
    }
    return next < maximum ? (s32)next : maximum;
}

static s32 ScaledCountStep(s32 step, s32 multiplier) {
    int64_t scaled;

    if (step <= 0) {
        step = 1;
    }
    scaled = (int64_t)step * multiplier;
    return scaled < INT_MAX ? (s32)scaled : INT_MAX;
}

/*
 * Move up to `step` of what is still owed into the player's money.
 *
 * The prize and the promotion bonus are counted in the same way, one from
 * PRIZE_SCREEN_STATE_COUNT_PRIZE and the other from COUNT_BONUS, and holding
 * the confirm button counts four times as fast.
 */
static void CountTowardsMoney(s32 *owed, s32 step) {
    s32 amount;
    s32 payment;

    if (owed == NULL) {
        return;
    }
    amount = *owed;
    if (amount <= 0 || g_RaceProgress == NULL) {
        *owed = 0;
        return;
    }
    if (step <= 0) {
        step = 1;
    }
    payment = amount < step ? amount : step;

    *owed = amount - payment;
    g_RaceProgress->money =
        CreditPrizeMoney(g_RaceProgress->money, payment);
}

/* Scene 19: counts the prize money and then the class-clear bonus into the save block. */
void UpdatePrizeMoneyScreenState(PrizeScreen *screen) {
    s32 multiplier = (g_PadHeld & PAD_CONFIRM)
        ? FAST_COUNT_MULTIPLIER
        : 1;
    s32 prizeStep = ScaledCountStep(screen->prizeStep, multiplier);
    s32 bonusStep = ScaledCountStep(screen->bonusStep, multiplier);

    switch (screen->state) {
    case PRIZE_SCREEN_STATE_INTRO_FADE_IN:
        screen->timer = AddClampedScreenValue(
            screen->timer, -PANEL_SLIDE_STEP, PRIZE_SCREEN_FADE_LIMIT);
        DrawFullscreenFadeTile(screen->timer, PRIZE_SCREEN_FADE_CLUT);
        if (screen->timer == 0) {
            screen->state = PRIZE_SCREEN_STATE_WAIT_FOR_INTRO_CONFIRM;
        }
        DrawRaceTimePanel(0);
        DrawGrandPrixIntro(0);
        return;
    case PRIZE_SCREEN_STATE_WAIT_FOR_INTRO_CONFIRM:
        DrawRaceTimePanel(0);
        if (g_PadPressed & PAD_CONFIRM) {
            screen->state = PRIZE_SCREEN_STATE_HIDE_RACE_TIME;
            screen->timer = 0;
        }
        DrawGrandPrixIntro(0);
        return;
    case PRIZE_SCREEN_STATE_HIDE_RACE_TIME:
        screen->timer = AddClampedScreenValue(
            screen->timer, PANEL_SLIDE_STEP,
            PANEL_OFFSCREEN_OFFSET + PANEL_SLIDE_STEP);
        DrawRaceTimePanel(screen->timer);
        if (screen->timer > PANEL_OFFSCREEN_OFFSET) {
            screen->state = PRIZE_SCREEN_STATE_SHOW_PRIZE_PANEL;
        }
        DrawGrandPrixIntro(0);
        return;
    case PRIZE_SCREEN_STATE_SHOW_PRIZE_PANEL:
        screen->timer = AddClampedScreenValue(
            screen->timer, -PANEL_SLIDE_STEP,
            PANEL_OFFSCREEN_OFFSET + PANEL_SLIDE_STEP);
        DrawPrizeMoneyPanel(screen->timer, screen->prize, screen->bonus);
        if (screen->timer == 0) {
            screen->state = PRIZE_SCREEN_STATE_COUNT_PRIZE;
        }
        DrawGrandPrixIntro(0);
        return;
    case PRIZE_SCREEN_STATE_COUNT_PRIZE:
        screen->timer = AddClampedScreenValue(
            screen->timer, 1, PRIZE_COUNT_DELAY_FRAMES + 1);
        /* The panel settles for two seconds before the counter starts. */
        if (screen->timer > PRIZE_COUNT_DELAY_FRAMES && screen->prize != 0) {
            PlaySoundCue((g_PadHeld & PAD_CONFIRM)
                             ? SOUND_CUE_FAST_COUNT
                             : SOUND_CUE_COUNT);
            CountTowardsMoney(&screen->prize, prizeStep);
        }
        if (screen->prize != 0) {
            break;
        }
        screen->timer = 0;
        screen->state = screen->bonus == 0
                                 ? PRIZE_SCREEN_STATE_WAIT_TO_FINISH
                                 : PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM;
        break;
    case PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM:
        PlaySoundCue(SOUND_CUE_CONFIRM);
        if (g_PadPressed & PAD_CONFIRM) {
            screen->state = PRIZE_SCREEN_STATE_COUNT_BONUS;
        }
        break;
    case PRIZE_SCREEN_STATE_COUNT_BONUS:
        TickClassClearFanfare();
        if (screen->bonus != 0) {
            PlaySoundCue((g_PadHeld & PAD_CONFIRM)
                             ? SOUND_CUE_FAST_COUNT
                             : SOUND_CUE_COUNT);
            CountTowardsMoney(&screen->bonus, bonusStep);
            if (screen->bonus != 0) {
                break;
            }
        }
        screen->state = PRIZE_SCREEN_STATE_WAIT_TO_FINISH;
        break;
    case PRIZE_SCREEN_STATE_WAIT_TO_FINISH:
        PlaySoundCue(SOUND_CUE_CONFIRM);
        if (!(g_PadPressed & PAD_CONFIRM) || TickClassClearFanfare() != 0) {
            break;
        }
        if (g_ClassCompleted == 0) {
            RequestSelectBgmAssets();
        }
        screen->state = PRIZE_SCREEN_STATE_FADE_OUT;
        break;
    case PRIZE_SCREEN_STATE_FADE_OUT:
        screen->timer = AddClampedScreenValue(
            screen->timer, g_SeriesCleared != 0 ? 1 : 2,
            PRIZE_SCREEN_FADE_LIMIT);
        DrawFullscreenFadeTile(screen->timer, PRIZE_SCREEN_FADE_CLUT);
        if (screen->timer >= PRIZE_SCREEN_FADE_LIMIT) {
            AdvanceGrandPrixClass();
        }
        break;
    default:
        DrawGrandPrixIntro(0);
        return;
    }
    DrawPrizeMoneyPanel(0, screen->prize, screen->bonus);
    DrawGrandPrixIntro(
        screen->state >= PRIZE_SCREEN_STATE_WAIT_FOR_BONUS_CONFIRM);
}

void UpdatePrizeMoneyScreen(void) {
    UpdatePrizeMoneyScreenState(&s_screen);
}
