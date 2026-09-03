#include "game/audio.h"
#include "game/car.h"
#include "game/course_index.h"
#include "game/integer.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"

enum {
    EXTENDED_RUBBER_BAND_COURSE = 3,
    DEFAULT_NEAR_DISTANCE = 0x600,
    DEFAULT_FAR_DISTANCE = 0xE00,
    EXTENDED_NEAR_DISTANCE = 0xC00,
    EXTENDED_FAR_DISTANCE = 0x1400,
    FAR_AHEAD_MINIMUM_SPEED = 0x321,
    NEAR_AHEAD_MINIMUM_SPEED = 0x3E9,
    FAR_AHEAD_ACCELERATION_PERCENT = 90,
    NEAR_AHEAD_ACCELERATION_PERCENT = 98,
    RIVAL_CUE_COOLDOWN = 0x12D,
    DISTANT_TRAILING_GAP = 0x1C00,
    NEAR_TRAILING_GAP = 0x7FF,
    RESET_TRAILING_CUE_GAP = 0x1000,
    REPEAT_TRAILING_CUE_GAP = 0x800,
    LEADER_TRAILING_CUE_BIT = 1,
    FIRST_NEAR_RIVAL_CUE_BIT = 0x10,
    FIRST_FAR_RIVAL_CUE_BIT = 0x100,
    CUE_LEADER_FAR_BEHIND = 0x2D,
    CUE_RIVAL_NEAR_BEHIND_A = 0x2F,
    CUE_RIVAL_NEAR_BEHIND_B = 0x30,
    CUE_RIVAL_CLOSE_AHEAD_FIRST = 0x32,
    CUE_RIVAL_REPEATED_BEHIND_A = 0x36,
    CUE_RIVAL_REPEATED_BEHIND_B = 0x37,
};

static s32 NearRivalCueBit(s32 rank) {
    return FIRST_NEAR_RIVAL_CUE_BIT >> rank;
}

static s32 FarRivalCueBit(s32 rank) {
    return FIRST_FAR_RIVAL_CUE_BIT >> rank;
}

static void PlayEnabledRivalCue(s32 cue) {
    if (g_RivalCueEnabled != 0) {
        PlaySoundCue(cue);
    }
}

static void UpdateTrailingRivalCue(s32 rank, s32 gap, s32 nearCueBit,
                                   s16 *cooldown) {
    if (rank == 0 && !(g_RivalCueFlags & LEADER_TRAILING_CUE_BIT) &&
        gap < -DISTANT_TRAILING_GAP) {
        if (g_PlayerCar.drive.racePosition == 1) {
            PlayEnabledRivalCue(CUE_LEADER_FAR_BEHIND);
        }
        g_RivalCueFlags =
            (g_RivalCueFlags & ~NearRivalCueBit(0)) |
            LEADER_TRAILING_CUE_BIT;
        return;
    }

    if (gap >= -NEAR_TRAILING_GAP &&
        !(nearCueBit & g_RivalCueFlags)) {
        PlayEnabledRivalCue(g_SceneTimer % 2
                                ? CUE_RIVAL_NEAR_BEHIND_A
                                : CUE_RIVAL_NEAR_BEHIND_B);
        g_RivalCueFlags |= nearCueBit;
        return;
    }

    if (gap < -RESET_TRAILING_CUE_GAP) {
        g_RivalCueFlags &= ~nearCueBit;
    } else if (gap < -REPEAT_TRAILING_CUE_GAP &&
               *cooldown >= RIVAL_CUE_COOLDOWN) {
        PlayEnabledRivalCue(g_SceneTimer % 2
                                ? CUE_RIVAL_REPEATED_BEHIND_B
                                : CUE_RIVAL_REPEATED_BEHIND_A);
        *cooldown = 0;
    }
}

void UpdateRivalRubberBand(void) {
    s32 playerProgress;
    s32 nearDistance;
    s32 farDistance;
    s32 rank;

    if (g_RacePhase >= 4) {
        return;
    }

    playerProgress = WrapSigned32(
        (int64_t)g_PlayerCar.progressA + g_PlayerCar.progressB);
    if (SeriesCourseIndex() == EXTENDED_RUBBER_BAND_COURSE) {
        nearDistance = EXTENDED_NEAR_DISTANCE;
        farDistance = EXTENDED_FAR_DISTANCE;
    } else {
        nearDistance = DEFAULT_NEAR_DISTANCE;
        farDistance = DEFAULT_FAR_DISTANCE;
    }

    g_ClosestRivalRank = -1;
    for (rank = RIVAL_CONTENDER_COUNT - 1; rank >= 0; rank--) {
        GameCarRuntime *rival = g_RankedCars[rank];
        s16 *cooldown = &g_RivalCueCooldowns[rank];
        s32 nearCueBit = NearRivalCueBit(rank);
        s32 farCueBit = FarRivalCueBit(rank);
        s32 gap;

        if (rival == NULL) {
            continue;
        }
        gap = WrapSigned32(
            (int64_t)rival->progressA + rival->progressB);
        gap = WrapSigned32((int64_t)gap - playerProgress);

        if (gap >= 0) {
            if (rank == 0) {
                g_RivalCueFlags &= ~LEADER_TRAILING_CUE_BIT;
            }
            g_ClosestRivalRank = rank;
            if (farDistance < gap) {
                g_RivalCueFlags &= ~farCueBit;
                if (rival->speed >= FAR_AHEAD_MINIMUM_SPEED) {
                    rival->accelerationLimit =
                        rival->accelerationLimit *
                        FAR_AHEAD_ACCELERATION_PERCENT / 100;
                }
                return;
            }
            if (nearDistance < gap) {
                if (rival->speed >= NEAR_AHEAD_MINIMUM_SPEED) {
                    rival->accelerationLimit =
                        rival->accelerationLimit *
                        NEAR_AHEAD_ACCELERATION_PERCENT / 100;
                }
                g_RivalCueFlags |= nearCueBit;
                if (*cooldown >= RIVAL_CUE_COOLDOWN) {
                    *cooldown = 0;
                }
                return;
            }
            if (!(farCueBit & g_RivalCueFlags)) {
                PlayEnabledRivalCue(CUE_RIVAL_CLOSE_AHEAD_FIRST +
                                    g_SceneTimer % 3);
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
