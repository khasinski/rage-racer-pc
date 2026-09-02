/*
 * How a rival decides which way to go round the car in front of it.
 *
 * The function sorts every other car that is ahead and close into three
 * buckets: to the left, roughly in front, and to the right. Each one is
 * weighted by how near it is, and the emptiest side becomes the offset the
 * rival steers towards. Being boxed in also damps how hard it may accelerate.
 *
 * It came back with names like acc8, acc9, k11, t6, a2 and carA4low, and
 * nothing covered it. This is a characterisation test: it does not say the
 * behaviour is right, only that it is what it was, so the shape can be
 * changed without the answers moving.
 *
 * The traffic is built here rather than driven, so the sweep is repeatable.
 */

#include "common.h"
#include "game/car.h"
#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"

#include <stdio.h>
#include <string.h>

void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex);

/* Everything the file reaches for that this sweep does not exercise. The
 * sound cue belongs to the rival chatter further down the same file. */
GameCarRuntime g_Cars[11];
PlayerCarRuntime g_PlayerCar;

void PlaySoundCue(s32 cue) { (void)cue; }

static PlayerCarRuntime s_playerStorage;
void *GetPlayerCarStorage(void) { return &s_playerStorage; }

enum { RIVAL_SLOTS = 11, TRACK_LENGTH = 0x8000 };

/* No second car at all, for the cases that do not need one. */
#define NO_BLOCKER 0x7FFF

static unsigned long s_digest = 2166136261UL;

static int CheckContenderRanking(void) {
    static const s32 cases[][4] = {
        {10, 40, 20, 30},
        {40, 30, 20, 10},
        {10, 20, 30, 40},
        {20, 20, 20, 20},
        {30, 10, 30, 10},
    };
    static const s32 expected[][4] = {
        {1, 3, 2, 0},
        {0, 1, 2, 3},
        {3, 2, 1, 0},
        {0, 1, 2, 3},
        {0, 2, 1, 3},
    };
    size_t test;

    for (test = 0; test < sizeof(cases) / sizeof(cases[0]); test++) {
        s32 rank;

        memset(g_Cars, 0, sizeof(g_Cars));
        for (rank = 0; rank < 4; rank++) {
            g_Cars[rank].progressA = cases[test][rank] - rank;
            g_Cars[rank].progressB = rank;
        }
        RankContenders();

        for (rank = 0; rank < 4; rank++) {
            s32 actual = (s32)(g_RankedCars[rank] - g_Cars);
            if (actual != expected[test][rank]) {
                printf("ranking case %lu rank %d selected car %d, expected %d\n",
                       (unsigned long)test, rank, actual, expected[test][rank]);
                return 1;
            }
        }
    }
    return 0;
}

static void Fold(FILE *out, const char *label, const GameCarRuntime *car) {
    const GameCarAiBlock *state = GetCarAiBlock((GameCarRuntime *)car);
    char line[256];
    const char *p;

    snprintf(line, sizeof(line),
             "%s -> step=%d active=%d target=%d lateral=%d nearby=%u limit=%d\n",
             label, (int)state->avoidanceStep, (int)state->avoidanceActive,
             (int)state->avoidanceTargetOffset, (int)state->aiLateralOffset,
             (unsigned)state->nearbyCarCount, (int)state->accelerationLimit);
    for (p = line; *p != '\0'; p++) {
        s_digest = (s_digest ^ (unsigned char)*p) * 16777619UL;
        s_digest &= 0xFFFFFFFFUL;
    }
    if (out != NULL) fputs(line, out);
}

/*
 * Four layouts, chosen to reach each way the buckets can fill: nobody at all,
 * a wall right across the road, traffic only on one side, and one car so close
 * that it lands in the separate near-miss count the function keeps.
 */
static void BuildTraffic(int layout, s32 subjectProgress, s32 subjectOffset) {
    s32 i;

    memset(g_Cars, 0, sizeof(g_Cars[0]) * RIVAL_SLOTS);
    for (i = 0; i < RIVAL_SLOTS; i++) g_Cars[i].activeFlag = -1;
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.trackProgress = subjectProgress + 0x900;
    g_PlayerCar.trackLateralOffset = subjectOffset;
    g_PlayerCar.speed = 0x120;

    if (layout == 0) return;

    for (i = 0; i < 6; i++) {
        GameCarRuntime *other = &g_Cars[i + 1];
        other->activeFlag = 0;
        other->speed = 0x100 + i * 0x40;
        if (layout == 1) {
            /* Spread across the width, so all three buckets take something. */
            other->trackLateralOffset = (s16)(subjectOffset - 0x28 + i * 0x10);
            other->trackProgress = subjectProgress + 0x300 + i * 0x180;
        } else if (layout == 2) {
            /* Everyone to one side, which is what makes the other side win. */
            other->trackLateralOffset = (s16)(subjectOffset + 0x18 + (i & 1) * 8);
            other->trackProgress = subjectProgress + 0x280 + i * 0x140;
        } else {
            /* Close enough to reach the near-miss counters and the wide ones. */
            other->trackLateralOffset =
                (s16)(subjectOffset + (i < 3 ? 0x45 : -0x45));
            other->trackProgress = subjectProgress + 0x80 + i * 0x40;
        }
    }
}

/*
 * One rival at a chosen distance and sideways offset, and nobody else. The
 * sweep above fills the road; this puts a single car exactly on a threshold,
 * which is the only way to tell one side of it from the other.
 */
static void RunOne(FILE *out, const char *what, s32 sceneId, s32 subjectOffset,
                   s32 subjectSpeed, s32 rivalAhead, s32 rivalSideways,
                   s32 playerAhead, s32 priorLimit, s32 blockerSideways,
                   int *cases) {
    GameCarRuntime *subject;
    GameCarAiBlock *state;
    char label[192];
    const s32 progress = 0x2000;

    g_SceneId = sceneId;
    memset(g_Cars, 0, sizeof(g_Cars[0]) * RIVAL_SLOTS);
    { s32 i; for (i = 0; i < RIVAL_SLOTS; i++) g_Cars[i].activeFlag = -1; }
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_PlayerCar.trackProgress = progress + playerAhead;
    g_PlayerCar.trackLateralOffset = (s16)subjectOffset;
    g_PlayerCar.speed = 0x120;

    g_Cars[2].activeFlag = 0;
    g_Cars[2].speed = 0;
    g_Cars[2].trackProgress = progress + rivalAhead;
    g_Cars[2].trackLateralOffset = (s16)(subjectOffset + rivalSideways);

    /*
     * The near-miss counters are only read once something has filled a
     * bucket, and only by the branch belonging to the emptiest side, so which
     * side the blocking car sits on decides which counter is consulted. A
     * case that needs one has to put the blocker opposite it.
     */
    if (blockerSideways != NO_BLOCKER) {
        g_Cars[3].activeFlag = 0;
        g_Cars[3].speed = 0;
        g_Cars[3].trackProgress = progress + 0x400;
        g_Cars[3].trackLateralOffset = (s16)(subjectOffset + blockerSideways);
    }

    subject = &g_Cars[5];
    subject->activeFlag = 0;
    subject->trackProgress = progress;
    subject->trackLateralOffset = (s16)subjectOffset;
    subject->speed = subjectSpeed;
    state = GetCarAiBlock(subject);
    state->accelerationLimit = (s16)priorLimit;
    state->aiLateralOffset = (s16)subjectOffset;

    snprintf(label, sizeof(label), "%s ahead=%d sideways=%d blocker=%d", what,
             rivalAhead, rivalSideways, (int)blockerSideways);
    UpdateCarTrafficAvoidance(subject, 5);
    Fold(out, label, subject);
    (*cases)++;
}

int main(int argc, char **argv) {
    /*
     * What the function did before it was touched. Run with a file name to
     * write the sweep out and diff two runs to see which cases moved.
     */
    static const unsigned long expected = 3400554701UL;
    static const s32 layouts[] = {0, 1, 2, 3};
    static const s32 subjectOffsets[] = {0, 0x60, -0x60, 0x51, -0x50};
    static const s32 subjectSpeeds[] = {0, 0x100, 0x400};
    static const s32 carIndices[] = {1, 3, 4, 10};
    static const s32 sceneIds[] = {0xC, 0x3};
    static const s16 priorActive[] = {0, 1};
    static const s16 priorLimits[] = {0, 100};
    FILE *out = NULL;
    size_t l, o, v, c, s, a, m;
    int cases = 0;

    if (CheckContenderRanking() != 0) return 1;

    if (argc > 1) {
        out = fopen(argv[1], "w");
        if (out == NULL) {
            fprintf(stderr, "cannot write %s\n", argv[1]);
            return 2;
        }
    }
    g_TrackLength = TRACK_LENGTH;

    for (l = 0; l < sizeof(layouts) / sizeof(layouts[0]); l++)
    for (o = 0; o < sizeof(subjectOffsets) / sizeof(subjectOffsets[0]); o++)
    for (v = 0; v < sizeof(subjectSpeeds) / sizeof(subjectSpeeds[0]); v++)
    for (c = 0; c < sizeof(carIndices) / sizeof(carIndices[0]); c++)
    for (s = 0; s < sizeof(sceneIds) / sizeof(sceneIds[0]); s++)
    for (a = 0; a < sizeof(priorActive) / sizeof(priorActive[0]); a++)
    for (m = 0; m < sizeof(priorLimits) / sizeof(priorLimits[0]); m++) {
        GameCarRuntime *subject;
        GameCarAiBlock *state;
        char label[192];
        const s32 progress = 0x2000;

        g_SceneId = sceneIds[s];
        BuildTraffic(layouts[l], progress, subjectOffsets[o]);

        subject = &g_Cars[carIndices[c]];
        subject->activeFlag = 0;
        subject->trackProgress = progress;
        subject->trackLateralOffset = (s16)subjectOffsets[o];
        subject->speed = subjectSpeeds[v];
        state = GetCarAiBlock(subject);
        state->avoidanceActive = priorActive[a];
        state->accelerationLimit = priorLimits[m];
        state->aiLateralOffset = (s16)subjectOffsets[o];

        snprintf(label, sizeof(label),
                 "layout=%d offset=%d speed=%d index=%d scene=%d active=%d "
                 "limit=%d",
                 layouts[l], subjectOffsets[o], subjectSpeeds[v],
                 carIndices[c], sceneIds[s], (int)priorActive[a],
                 (int)priorLimits[m]);
        UpdateCarTrafficAvoidance(subject, carIndices[c]);
        Fold(out, label, subject);
        cases++;
    }

    /*
     * Thresholds, each visited from both sides. Nothing in the sweep above
     * lands exactly on one, and a threshold nobody stands on is a threshold
     * that can be moved without anything noticing.
     */
    {
        static const s32 nearMiss[] = {0x3F, 0x40, 0x41, 0x42,
                                       -0x3F, -0x40, -0x41, -0x42};
        /* A stationary car looks 0xC00 ahead and contributes that minus the
         * gap, so these gaps weigh 0x3EA, 0x3E9, 0x3E8 and 0x3E7: either side
         * of the 0x3E9 at which acceleration starts being damped. */
        static const s32 damping[] = {0x816, 0x817, 0x818, 0x819};
        static const s32 nearBoundary[] = {0x1FF, 0x200, 0x201};
        /* Counted as nearby when it is within 0x400 of the line behind. */
        static const s32 behind[] = {0x380, 0x400, 0x401, 0x450, 0x500};
        /* The player is looked for over a range of its own. */
        static const s32 playerGaps[] = {0x14A0, 0x1500, 0x1560, 0x1561};
        /* Far enough out that the sideways window wraps a signed word. */
        static const s32 extremes[] = {32700, -32700};
        size_t k;

        for (k = 0; k < sizeof(nearMiss) / sizeof(nearMiss[0]); k++) {
            RunOne(out, "near miss, blocked left", 3, 0, 0, 0x100, nearMiss[k],
                   0x900, 0, -0x20, &cases);
            RunOne(out, "near miss, blocked right", 3, 0, 0, 0x100, nearMiss[k],
                   0x900, 0, 0x20, &cases);
        }
        /* The near-miss counters only look this far ahead. */
        for (k = 0; k < sizeof(nearBoundary) / sizeof(nearBoundary[0]); k++) {
            RunOne(out, "near limit, blocked left", 3, 0, 0, nearBoundary[k],
                   0x45, 0x900, 0, -0x20, &cases);
            RunOne(out, "near limit, blocked right", 3, 0, 0, nearBoundary[k],
                   -0x45, 0x900, 0, 0x20, &cases);
        }
        for (k = 0; k < sizeof(damping) / sizeof(damping[0]); k++)
            RunOne(out, "damping", 3, 0, 0, damping[k], 0, 0x900, 100, NO_BLOCKER,
                   &cases);
        for (k = 0; k < sizeof(behind) / sizeof(behind[0]); k++)
            RunOne(out, "behind", 3, 0, 0, -behind[k], 0, 0x900, 0, NO_BLOCKER, &cases);
        for (k = 0; k < sizeof(playerGaps) / sizeof(playerGaps[0]); k++)
            RunOne(out, "player gap", 0xC, 0, 0, 0x4000, 0, playerGaps[k], 0,
                   NO_BLOCKER, &cases);
        for (k = 0; k < sizeof(extremes) / sizeof(extremes[0]); k++) {
            RunOne(out, "far out", 3, extremes[k], 0, 0x100, 0x10, 0x900, 0, NO_BLOCKER,
                   &cases);
            RunOne(out, "far out wrapped", 3, extremes[k], 0, 0x100,
                   -2 * extremes[k], 0x900, 0, NO_BLOCKER, &cases);
        }
    }

    if (out != NULL) fclose(out);
    if (s_digest != expected) {
        printf("traffic_avoidance: %d cases folded to %lu, expected %lu\n",
               cases, s_digest, expected);
        return 1;
    }

    /* The retail counter increments through an unsigned halfword view. Keep
     * the wrap defined when the decompiler's pointer union is removed. */
    memset(g_Cars, 0, sizeof(g_Cars));
    memset(&g_PlayerCar, 0, sizeof(g_PlayerCar));
    g_RankedCars[0] = &g_Cars[0];
    g_RankedCars[1] = &g_Cars[1];
    g_RankedCars[2] = &g_Cars[2];
    g_RankedCars[3] = &g_Cars[3];
    g_CourseIndex = 0;
    g_RacePhase = 2;
    g_RivalCueFlags = 0x20;
    g_RivalCueCooldown3 = 0x7FFF;
    UpdateRivalRubberBand();
    if (g_RivalCueCooldown3 != (s16)0x8000) {
        printf("rival cue cooldown did not wrap: %d\n",
               g_RivalCueCooldown3);
        return 1;
    }

    /* Each rank owns an independent cooldown. These globals are deliberately
     * not required to be adjacent in host state. */
    {
        s16 *cooldowns[4] = {
            &g_RivalCueCooldown0, &g_RivalCueCooldown1,
            &g_RivalCueCooldown2, &g_RivalCueCooldown3,
        };
        s32 rank;

        for (rank = 0; rank < 4; rank++) {
            s32 i;

            for (i = 0; i < 4; i++) {
                g_Cars[i].progressA = i > rank ? -0x2000 : 0;
                *cooldowns[i] = (s16)(100 + i);
            }
            g_RivalCueFlags = 0x1E0;
            UpdateRivalRubberBand();
            for (i = 0; i < 4; i++) {
                s16 expectedCooldown =
                    (s16)(100 + i + (i == rank ? 1 : 0));
                if (*cooldowns[i] != expectedCooldown) {
                    printf("rank %d changed cooldown %d to %d, expected %d\n",
                           rank, i, *cooldowns[i], expectedCooldown);
                    return 1;
                }
            }
        }
    }

    printf("traffic_avoidance: %d cases unchanged\n", cases);
    return 0;
}
