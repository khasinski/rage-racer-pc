#include "game/audio.h"
#include "game/car.h"
#include "game/course_index.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"

static s16 *RivalCueCooldown(s32 rank) {
    switch (rank) {
    case 0:
        return &g_RivalCueCooldown0;
    case 1:
        return &g_RivalCueCooldown1;
    case 2:
        return &g_RivalCueCooldown2;
    default:
        return &g_RivalCueCooldown3;
    }
}

static void PlayEnabledRivalCue(s32 cue) {
    if (g_RivalCueEnabled != 0) {
        PlaySoundCue(cue);
    }
}

static void UpdateTrailingRivalCue(s32 rank, s32 gap, s32 nearCueBit,
                                   s16 *cooldown) {
    if (rank == 0 && !(g_RivalCueFlags & 1) && gap < -0x1C00) {
        if (g_RacePosition == 1) {
            PlayEnabledRivalCue(0x2D);
        }
        g_RivalCueFlags = (g_RivalCueFlags & ~0x10) | 1;
        return;
    }

    if (gap >= -0x7FF && !(nearCueBit & g_RivalCueFlags)) {
        PlayEnabledRivalCue(g_SceneTimer % 2 ? 0x2F : 0x30);
        g_RivalCueFlags |= nearCueBit;
        return;
    }

    if (gap < -0x1000) {
        g_RivalCueFlags &= ~nearCueBit;
    } else if (gap < -0x800 && *cooldown >= 0x12D) {
        PlayEnabledRivalCue(g_SceneTimer % 2 ? 0x37 : 0x36);
        *cooldown = 0;
    }
}

void UpdateRivalRubberBand(void) {
    s32 playerProgress;
    s32 nearDistance;
    s32 farDistance;
    s32 rank;

    playerProgress = g_PlayerCar.progressA + g_PlayerCar.progressB;
    if (SeriesCourseIndex() == 3) {
        nearDistance = 0xC00;
        farDistance = 0x1400;
    } else {
        nearDistance = 0x600;
        farDistance = 0xE00;
    }

    if (g_RacePhase >= 4) {
        return;
    }

    g_ClosestRivalRank = -1;
    for (rank = 3; rank >= 0; rank--) {
        GameCarRuntime *rival = g_RankedCars[rank];
        s16 *cooldown = RivalCueCooldown(rank);
        s32 nearCueBit = 0x10 >> rank;
        s32 farCueBit = 0x100 >> rank;
        s32 gap;

        if (rival == NULL) {
            continue;
        }
        gap = rival->progressA + rival->progressB - playerProgress;

        if (gap >= 0) {
            if (rank == 0) {
                g_RivalCueFlags &= ~1;
            }
            g_ClosestRivalRank = rank;
            if (farDistance < gap) {
                g_RivalCueFlags &= ~farCueBit;
                if (rival->speed >= 0x321) {
                    rival->accelerationLimit =
                        rival->accelerationLimit * 90 / 100;
                }
                return;
            }
            if (nearDistance < gap) {
                s32 counter;

                if (rival->speed >= 0x3E9) {
                    rival->accelerationLimit =
                        rival->accelerationLimit * 98 / 100;
                }
                counter = *cooldown;
                g_RivalCueFlags |= nearCueBit;
                if (counter >= 0x12D) {
                    *cooldown = 0;
                }
                return;
            }
            if (!(farCueBit & g_RivalCueFlags)) {
                PlayEnabledRivalCue(0x32 + g_SceneTimer % 3);
                *cooldown = 0;
                g_RivalCueFlags |= farCueBit;
                return;
            }
            *cooldown = (s16)((u16)*cooldown + 1);
            return;
        }

        UpdateTrailingRivalCue(rank, gap, nearCueBit, cooldown);
    }
}
