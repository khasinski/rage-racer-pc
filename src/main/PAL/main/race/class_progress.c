#include <stdio.h>
#include "game/race.h"
#include "game/state.h"
#include "game/audio.h"
#include "game/sound.h"
#include "game/render.h"
#include "game/save_internal.h"

void DrawPrizeMoneyPanel(s32 s0) {
    char sp[16];
    if (g_RaceProgress->money.value > 0x3B9AC9FF) {
        g_RaceProgress->money.value = 0x3B9AC9FF;
    }
    DrawProportionalText(0x10, s0 + 128, g_CaptionPrizeMoney, 0x7812);
    sprintf(sp, g_FmtMoney, g_PrizeAmount);
    DrawProportionalText(0x12, s0 + 140, sp, 0x7812);
    DrawProportionalText(0x10, s0 + 160, g_CaptionTotalMoney, 0x7812);
    sprintf(sp, g_FmtMoney, g_RaceProgress->money.value);
    DrawProportionalText(0x12, s0 + 172, sp, 0x7812);
    if (g_ClassPromoted != 0) {
        DrawProportionalText(0x10, s0 + 192, g_CaptionPromotionBonus, 0x7812);
        sprintf(sp, g_FmtMoney, g_PromotionBonus);
        DrawProportionalText(0x12, s0 + 204, sp, 0x7812);
    }
}

void CommitClassProgress(void) {
    s32 score_index;
    u8 *slots;
    s32 slot_count;
    s32 filled;
    s32 i;
    s32 done;
    s32 value;
    GameRaceProgress *state;

    slots = &g_CourseProgress->bestPlace[SeriesCourseIndex()];
    g_ClassClearFanfareTimer = 0;

    if (*slots == 0 || g_RacePosition < *slots) {
        *slots = g_RacePosition;
    }

    value = GetCarUnlockLevel(g_PlayerCarIndex);
    slot_count = 4;
    if (g_GrandPrixClass < value) {
        g_CourseProgress->unlockPending = 1;
    }

    if (g_GrandPrixClass < 2) {
        slot_count = 3;
    }

    filled = 0;
    for (i = 0; i < slot_count; i++) {
        if (g_CourseProgress->bestPlace[i] != 0) {
            filled++;
        }
    }

    done = slot_count == filled;
    g_ClassCompleted = done;

    if (done != 0) {
        score_index = (g_GrandPrixSeries * 6) + g_GrandPrixClass;

        if (score_index == 4) {
            if (g_ClassRecords[6].place == -1) {
                g_ClassRecords[6].place = 0;
            }
        } else if (score_index == 10) {
            if (g_ClassRecords[5].place == -1) {
                g_ClassRecords[5].place = 0;
            }
        } else {
            if (score_index != 5) {
                if (g_ClassRecords[score_index + 1].place == -1) {
                    g_ClassRecords[score_index + 1].place = 0;
                }
            }
        }
        value = ComputeClassGrade();
        g_ClassResultPlace = value;
        if (value != 0) {
            if (g_ClassRecords[score_index].place == 0 || value < g_ClassRecords[score_index].place) {
                g_ClassRecords[score_index].place = (u16)g_ClassResultPlace;
            }
            g_ClassClearFanfareTimer = 0xD2;
        }

        UpdateBgmTrackCount();
        if (g_ClassResultPlace == 1) {
            if ((s16)g_ClassRecords[score_index].clears < 99) {
                g_ClassRecords[score_index].clears++;
            }
        }
    } else {
        g_ClassResultPlace = 0;
    }

    g_SeriesCleared = 0;
    if (g_ClassCompleted != 0) {
        if ((g_SeriesSelection == 0 && g_GrandPrixClass == 4) || (g_SeriesSelection == 1 && g_GrandPrixClass == 5)) {
            g_SeriesCleared = 1;
            g_ExtraGrandPrixUnlocked = 1;
        }
    }

    g_ClassPromoted = 0;
    if (g_ClassCompleted != 0 && g_SeriesCleared == 0) {
        state = g_RaceProgress;
        if (state->maxClassReached < g_GrandPrixClass + 1) {
            g_ClassPromoted = 1;
        }
    }
}

void AdvanceGrandPrixClass(void) {
    s32 oldValue;
    GameRaceProgress *ptr;
    s32 *entry;

    if (g_ClassCompleted != 0) {
        if (g_SeriesCleared != 0) {
            ptr = g_RaceProgress;
            oldValue = ptr->maxClassReached;
            ResetProgressSlot(g_CarTable, ptr);
            g_RaceProgress->money.value = 0x3B9AC9FF;
            g_RaceProgress->maxClassReached = oldValue;
            ResetCourseProgress(0);
            BeginEndingFmv(0x21);
        } else {
            s32 next;

            BeginClassFmv(7);
            next = g_GrandPrixClass + 1;
            g_GrandPrixClass = next;
            g_RaceProgress->classIndex = next;
            g_RaceProgress->course = 0;

            if (g_ClassPromoted != 0) {
                g_RaceProgress->maxClassReached = next;
                entry = &g_MaxClassReached[g_SeriesSelection];
                if (*entry < next) {
                    *entry = next;
                }
            }

            ResetCourseProgress(g_GrandPrixClass);
        }
    } else {
        g_SceneId = 6;
    }
}

void EnterPrizeScreen(void) {
    s32 mode;
    s32 car;
    s32 value;

    g_SceneTimer = 0x100;
    g_FrameSyncThreshold = 0x80;

    mode = g_CourseIndex;
    car = g_GrandPrixClass;
    g_PrizeScreenState = PRIZE_SCREEN_STATE_INTRO_FADE_IN;
    g_PrizeAmount = g_PrizeMoney.values[mode][car][g_RacePosition - 1];
    g_SceneId = 0x13;

    if (g_ClassPromoted != 0) {
        g_PromotionBonus = g_PromotionBonusTable[car];
    } else {
        g_PromotionBonus = 0;
    }

    value = g_PrizeMoney3rd[g_CourseIndex][g_GrandPrixClass][0] / 80;
    g_PrizeCountStep = value;
    if (value <= 0) {
        g_PrizeCountStep = 1;
    }

    value = g_PromotionBonusTable[g_GrandPrixClass] / 250;
    g_BonusCountStep = value;
    if (value <= 0) {
        g_BonusCountStep = 1;
    }
}

void TickClassClearFanfare(void) {
    if (g_ClassClearFanfareTimer > 0) {
        g_ClassClearFanfareTimer--;
    }
    if (g_ClassClearFanfareTimer == 0xB4) {
        PlaySoundCue(0x42);
    }
}
