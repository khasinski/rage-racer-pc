#include "game/player_car_internal.h"
#include "game/state.h"
#include "game/track.h"
#include "game/race.h"
#include "game/audio.h"

/*
 * Slot 11 holds the player; the rivals fill 0 to 10. Outside the race scene
 * the player is not on the road to be avoided, so the walk stops before it.
 */
enum {
    TRAFFIC_PLAYER_SLOT = 0xB,
    TRAFFIC_SLOT_COUNT = 12,
    TRAFFIC_RACE_SCENE = 0xC,
    /* Half the width of the lane a car watches directly in front of itself. */
    TRAFFIC_LANE_HALF_WIDTH = 0x30,
    /* Beyond this to the side is not in the way, but is close enough to stop
     * this car steering that way. */
    TRAFFIC_SHOULDER = 0x40,
    /* How far back down the road another car still counts as nearby. */
    TRAFFIC_BEHIND = 0x400,
    /* Where the rival aims once it has picked a side. */
    TRAFFIC_TARGET_OFFSET = 0x50,
};

/*
 * Decides which way a rival goes round the traffic in front of it.
 *
 * Every other car that is ahead and close enough to matter falls into one of
 * three buckets by where it sits across the road: to the left, in front, or to
 * the right. Each contributes its nearness, so a car about to be reached
 * weighs more than one at the edge of sight, and the emptiest bucket is the
 * side the rival steers towards. Two separate counts watch the ground just off
 * either shoulder, and a side with anything on it is not chosen however empty
 * its bucket looks. Being hemmed in on all three also damps how hard this car
 * is allowed to accelerate.
 *
 * Nothing here moves the car: it leaves behind an offset to aim at and a step
 * to get there by, and the steering does the rest.
 */
void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex) {
    /* The two views are the same memory, so which one a field is reached
     * through makes no difference; the recovered code used both. */
    GameCarAiBlock *state = GetCarAiBlock(car);
    s32 trackLength = g_TrackLength;
    s32 progress = car->trackProgress;
    s32 lateralOffset = car->trackLateralOffset;
    s32 speed = (u16)car->speed;
    /* The faster this car is going, the further ahead it looks. */
    s32 ownLookahead = car->speed * 2 + 0xC00;
    s32 laneLeft = (s16)(lateralOffset - TRAFFIC_LANE_HALF_WIDTH);
    s32 laneRight = (s16)(lateralOffset + TRAFFIC_LANE_HALF_WIDTH);
    s32 behindLine = trackLength - TRAFFIC_BEHIND;
    /* Left, in front, right. */
    s32 lane[3];
    s32 blockingLeft = 0;
    s32 blockingRight = 0;
    s32 crowding;
    s32 urgency;
    s32 slot;

    lane[0] = lane[1] = lane[2] = 0;
    state->avoidanceStep = 0;
    state->nearbyCarCount = 0;

    for (slot = 0; slot < TRAFFIC_SLOT_COUNT; slot++) {
        s32 otherOffset;
        s32 otherSpeed;
        s32 otherProgress;
        s32 alreadyAvoiding;
        /* How far ahead to look for this particular car. */
        s32 lookahead = ownLookahead;
        s32 sideways;
        s32 gap;

        if (g_SceneId != TRAFFIC_RACE_SCENE && slot == TRAFFIC_PLAYER_SLOT)
            break;
        if (slot == carIndex) continue;

        if (slot == TRAFFIC_PLAYER_SLOT) {
            otherProgress = g_PlayerCar.trackProgress + trackLength;
            otherOffset = g_PlayerCar.trackLateralOffset;
            /* The four cars at the front of the field are not told how fast
             * the player is going, so they never treat it as pulling away. */
            otherSpeed = carIndex < 4 ? 0 : (u16)g_PlayerCar.speed;
            alreadyAvoiding = 0;
            /* The player is watched over a range of its own, and that range
             * shrinks as the player speeds up rather than growing. */
            lookahead = 0x1800 - g_PlayerCar.speed * 2;
        } else {
            GameCarRuntime *other = &g_Cars[slot];
            if (other->activeFlag == -1) continue;
            otherProgress = other->trackProgress + trackLength;
            otherOffset = other->trackLateralOffset;
            otherSpeed = (u16)other->speed;
            alreadyAvoiding = (u16)state->avoidanceActive;
        }

        sideways = otherOffset - lateralOffset;
        gap = (otherProgress - progress) % trackLength;

        if (gap <= 0 || gap >= lookahead) {
            /* Behind, but not far behind. */
            if (behindLine < gap) state->nearbyCarCount++;
            continue;
        }

        state->nearbyCarCount++;

        if (laneLeft < otherOffset && otherOffset < laneRight) {
            /*
             * A car that is pulling away is not in the way. One already being
             * avoided stays in the way even if it is, so that a rival part way
             * round something does not change its mind halfway.
             */
            s32 closing = otherSpeed - speed;
            if (!((s16)closing > 0 && alreadyAvoiding == 0)) {
                /*
                 * The lane test above holds the offset inside the lane, so
                 * this always picks 0, 1 or 2. The recovered code carried a
                 * correction for a negative index that nothing can reach:
                 * measured over the sweep and over all four courses, the value
                 * stayed between 1 and 95 of a possible 0 to 95.
                 */
                s32 bucket = (sideways + TRAFFIC_LANE_HALF_WIDTH) >> 5;
                state->avoidanceActive = 1;
                if (slot == TRAFFIC_PLAYER_SLOT) {
                    /* The player counts full weight until it is inside 0xC00,
                     * and nearness only tells beyond that. */
                    lane[bucket] += gap < 0xC00 ? 0xC00 - gap : 0xC00;
                } else {
                    lane[bucket] += lookahead - gap;
                }
            }
        }

        /* Just off either shoulder and almost alongside: not in the way, but
         * no room to move over either. */
        if (gap < 0x200) {
            if ((s16)sideways >= TRAFFIC_SHOULDER + 1)
                blockingRight += 0xC00 - gap;
            else if ((s16)sideways < -TRAFFIC_SHOULDER)
                blockingLeft += 0xC00 - gap;
        }
    }

    crowding = lane[0] + lane[1] + lane[2];
    if (crowding <= 0) {
        state->avoidanceStep = 0;
        state->avoidanceActive = 0;
        state->avoidanceTargetOffset = state->aiLateralOffset;
        state->aiLateralOffset = state->aiLateralOffset + state->avoidanceStep;
        return;
    }

    /*
     * Already out towards one edge with nothing on the other side is reason
     * enough to cross, and it is crossed harder than a choice made on the
     * buckets alone. Otherwise the emptiest side wins, if it is free.
     */
    urgency = state->avoidanceActive;
    if (blockingLeft == 0 && lateralOffset >= TRAFFIC_TARGET_OFFSET + 1) {
        state->avoidanceTargetOffset = -TRAFFIC_TARGET_OFFSET;
        state->avoidanceStep = -8 - urgency * 2;
    } else if (blockingRight == 0 &&
               lateralOffset < -TRAFFIC_TARGET_OFFSET) {
        state->avoidanceTargetOffset = TRAFFIC_TARGET_OFFSET;
        state->avoidanceStep = 8 + urgency * 2;
    } else if (lane[0] <= lane[1] && lane[0] <= lane[2] &&
               blockingLeft == 0) {
        state->avoidanceTargetOffset = -TRAFFIC_TARGET_OFFSET;
        state->avoidanceStep = -6 - urgency * 2;
    } else if (lane[2] <= lane[1] && lane[2] <= lane[0] &&
               blockingRight == 0) {
        state->avoidanceTargetOffset = TRAFFIC_TARGET_OFFSET;
        state->avoidanceStep = 6 + urgency * 2;
    }

    /* Boxed in badly enough, and the car is not allowed to press on as hard.
     * Thirty hundredths, written as fifteen doubled. */
    if (crowding >= 0x3E9)
        state->accelerationLimit = (s16)(state->accelerationLimit * 30 / 100);

    state->aiLateralOffset = state->aiLateralOffset + state->avoidanceStep;
}

void SlowRivalAhead(GameCarRuntime *car, s32 carIndex) {
    GameCarRuntime *entry;
    s32 pos0Base;
    s32 pos0;
    s32 pos1;
    s32 value;

    pos0Base = car->progressA;
    entry = g_RankedCars[carIndex - 1];
    pos0 = pos0Base + car->progressB;
    pos1 = entry->progressA + entry->progressB;

    if ((pos1 - pos0) < 0x2800) {
        return;
    }

    if (entry->speed >= 0x385) {
        value = entry->accelerationLimit;
        value = ((value * 5) + ((value * 5) << 4)) / 100;
        entry->accelerationLimit = value;
    }
}

/*
 * Ranks the first four cars by race progress (`progressA + progressB`) and
 * publishes the ordering into g_RankedCars: slot 0 the leader, slot 3 the
 * last of the four, slots 1/2 the middle pair in order. UpdateRivalRubberBand reads
 * the result to rubber-band the AI.
 */
void RankContenders(void) {
    s32 i;
    s32 maxValue;
    s32 minValue;
    s32 value;
    s32 sums[4];
    s16 indices[4] = {0, 0, 0, 0};

    for (i = 0; i < 4; i++) {
        sums[i] = g_Cars[i].progressA + g_Cars[i].progressB;
    }

    indices[0] = 0;
    indices[3] = 0;
    maxValue = sums[0];
    minValue = sums[0];
    for (i = 1; i < 4; i++) {
        value = sums[i];
        if (maxValue < value) {
            maxValue = value;
            indices[0] = i;
        } else if (value < minValue) {
            minValue = value;
            indices[3] = i;
        }
    }

    g_RankedCars[0] = &g_Cars[indices[0]];
    g_RankedCars[3] = &g_Cars[indices[3]];

    for (i = 0; i < 4; i++) {
        if ((i != indices[0]) && (i != indices[3])) {
            indices[1] = i;
            break;
        }
    }

    for (i = 0; i < 4; i++) {
        if ((i != indices[0]) && (i != indices[3]) && (i != indices[1])) {
            indices[2] = i;
            break;
        }
    }

    if (sums[indices[1]] > sums[indices[2]]) {
        g_RankedCars[1] = &g_Cars[indices[1]];
        g_RankedCars[2] = &g_Cars[indices[2]];
    } else {
        g_RankedCars[1] = &g_Cars[indices[2]];
        g_RankedCars[2] = &g_Cars[indices[1]];
    }
}


static s16 *RivalCueCooldown(s32 rank) {
    switch (rank) {
    case 0: return &g_RivalCueCooldown0;
    case 1: return &g_RivalCueCooldown1;
    case 2: return &g_RivalCueCooldown2;
    default: return &g_RivalCueCooldown3;
    }
}

void UpdateRivalRubberBand(void) {
    s32 playerProgress;
    s32 nearDistance;
    s32 farDistance;
    s32 rank;

    playerProgress = g_PlayerCar.progressA + g_PlayerCar.progressB;
    if ((SeriesCourseIndex()) == 3) {
        nearDistance = 0xC00;
        farDistance = 0x1400;
    } else {
        nearDistance = 0x600;
        farDistance = 0xE00;
    }

    if (g_RacePhase >= 4) {
        return;
    }

    for (rank = 3; rank >= 0; rank--) {
        GameCarRuntime *rival = g_RankedCars[rank];
        s16 *cooldown = RivalCueCooldown(rank);
        s32 nearCueBit = 0x10 >> rank;
        s32 farCueBit = 0x100 >> rank;
        s32 gap = rival->progressA + rival->progressB - playerProgress;

        if (gap >= 0) {
            if (rank == 0) {
                g_RivalCueFlags &= ~1;
            }
            g_ClosestRivalRank = rank;
            if (farDistance < gap) {
                g_RivalCueFlags &= ~farCueBit;
                if (rival->speed >= 0x321) {
                    rival->accelerationLimit = rival->accelerationLimit * 90 / 100;
                }
                return;
            }
            if (nearDistance < gap) {
                s32 counter;

                if (rival->speed >= 0x3E9) {
                    rival->accelerationLimit = rival->accelerationLimit * 98 / 100;
                }
                counter = *cooldown;
                g_RivalCueFlags |= nearCueBit;
                if (counter >= 0x12D) {
                    *cooldown = 0;
                }
                return;
            }
            if (!(farCueBit & g_RivalCueFlags)) {
                s32 flags;
                u32 cueVariant;

                cueVariant = g_SceneTimer;
                switch (cueVariant % 3) {
                default:
                case 0:
                if (g_RivalCueEnabled != 0) {
                    PlaySoundCue(0x32);
                }
                    break;
                case 1:
                if (g_RivalCueEnabled != 0) {
                    PlaySoundCue(0x33);
                }
                    break;
                case 2:
                if (g_RivalCueEnabled != 0) {
                    PlaySoundCue(0x34);
                }
                }

                flags = g_RivalCueFlags;
                *cooldown = 0;
                g_RivalCueFlags = farCueBit | flags;
                return;
            }
            *cooldown = (s16)((u16)*cooldown + 1);
            return;
        } else {
            if (rank == 0 && !(g_RivalCueFlags % 2) && gap < -0x1C00) {
                if (g_RivalCueEnabled != 0 && g_RacePosition == 1) {
                    PlaySoundCue(0x2D);
                }
                g_RivalCueFlags = (g_RivalCueFlags & ~0x10) | 1;
            } else if (gap >= -0x7FF && !(nearCueBit & g_RivalCueFlags)) {
                if (g_SceneTimer % 2) {
                    if (g_RivalCueEnabled != 0) PlaySoundCue(0x2F);
                } else {
                    if (g_RivalCueEnabled != 0) PlaySoundCue(0x30);
                }
                g_RivalCueFlags |= nearCueBit;
            } else {
                if (gap < -0x1000) {
                    g_RivalCueFlags &= ~nearCueBit;
                } else if (gap < -0x800) {
                    if (*cooldown >= 0x12D) {
                        if (g_SceneTimer % 2) {
                            if (g_RivalCueEnabled != 0) PlaySoundCue(0x37);
                        } else {
                            if (g_RivalCueEnabled != 0) PlaySoundCue(0x36);
                        }
                        *cooldown = 0;
                    }
                }
            }
        }
    }
}
