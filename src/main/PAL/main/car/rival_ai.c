#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/player_car_internal.h"
#include "game/state.h"
#include "game/track.h"
#include "game/race.h"
#include "game/audio.h"

typedef union RivalCueCooldownAddress {
    s16 *signedCounter;
    u16 *unsignedCounter;
} RivalCueCooldownAddress;

void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex) {
    GameCarAiBlock *state = GetCarAiBlock(car);
    s32 acc8 = 0;
    s32 acc9 = 0;
    register s32 i asm("$10") = 0;
    s32 k11 = 0xB;
    s32 carProgress;
    s32 carLateralOffset;
    s32 carA4low;
    register s32 track asm("$12");
    s32 t6;
    s32 lateralMin;
    s32 lateralMax;
    s32 trackMinus;
    s32 total;
    s32 sums[4];

    carProgress = car->trackProgress;
    carLateralOffset = car->trackLateralOffset;
    carA4low = (u16)car->speed;
    car->avoidanceStep = 0;
    car->nearbyCarCount = 0;
    sums[3] = 0;
    sums[2] = 0;
    sums[1] = 0;
    sums[0] = 0;
    track = g_TrackLength;
    {
        s32 tmp = car->speed * 2;
        t6 = tmp + 0xC00;
    }
    lateralMin = (s16)(carLateralOffset - 0x30);
    lateralMax = (s16)(carLateralOffset + 0x30);
    trackMinus = track - 0x400;

    for (; i < 12; i++) {
        s32 otherLateralOffset;
        s32 otherA4;
        s32 a2;
        register s32 t1 asm("$9");
        s32 diff;
        s32 angleDiff;
        s32 angleSaved;
        s16 bucket;
        s32 val;

        if (g_SceneId != 0xC && i == k11) {
            break;
        }
        if (i == carIndex) {
            continue;
        }
        if (i == k11) {
            s32 op = g_PlayerCar.trackProgress;
            a2 = op + track;
            otherLateralOffset = g_PlayerCar.trackLateralOffset;
            if (carIndex < 4) {
                otherA4 = 0;
            } else {
                otherA4 = (u16)g_PlayerCar.speed;
            }
            t1 = 0;
            t6 = 0x1800 - (g_PlayerCar.speed * 2);
        } else {
            GameCarRuntime *other = &g_Cars[i];
            s32 op;
            if (other->activeFlag == -1) continue;
            otherLateralOffset = other->trackLateralOffset;
            otherA4 = (u16)other->speed;
            op = other->trackProgress;
            a2 = op + track;
            t1 = (u16)state->avoidanceActive;
        }

        angleDiff = otherLateralOffset - carLateralOffset;
        diff = (a2 - carProgress) % track;
        angleSaved = angleDiff;
        __asm__("" : "=r"(angleSaved) : "0"(angleSaved));
        otherA4 -= carA4low;

        if (diff > 0 && diff < t6) {
            state->nearbyCarCount++;
            if (lateralMin < otherLateralOffset && otherLateralOffset < lateralMax) {
                if (!((s16)otherA4 > 0 && t1 == 0)) {
                    state->avoidanceActive = 1;
                    val = angleDiff + 0x30;
                    if (val < 0) {
                        val = angleDiff + 0x4F;
                    }
                    bucket = val >> 5;
                    if (i == k11) {
                        if (diff < 0xC00) {
                            s32 s = sums[bucket] + 0xC00;
                            sums[bucket] = s - diff;
                        } else {
                            sums[bucket] = sums[bucket] + 0xC00;
                        }
                    } else {
                        sums[bucket] = (t6 - diff) + sums[bucket];
                    }
                }
            }
            if (diff < 0x200) {
                s32 ad = (s16)angleSaved;
                if (ad >= 0x41) {
                    s32 s = acc9 + 0xC00;
                    acc9 = s - diff;
                } else if (ad < -0x40) {
                    s32 s = acc8 + 0xC00;
                    acc8 = s - diff;
                }
            }
        } else {
            if (trackMinus < diff) {
                state->nearbyCarCount++;
            }
        }
    }

    total = sums[0] + sums[1] + sums[2];
    sums[3] = total;
    if (total > 0) {
        if (acc8 == 0 && carLateralOffset >= 0x51) {
            s32 avoidanceStrength = state->avoidanceActive;
            state->avoidanceTargetOffset = -0x50;
            state->avoidanceStep = -8 - (avoidanceStrength * 2);
        } else if (acc9 == 0 && carLateralOffset < -0x50) {
            s32 avoidanceStrength = state->avoidanceActive;
            state->avoidanceTargetOffset = 0x50;
            state->avoidanceStep = (avoidanceStrength * 2) + 8;
        } else if (sums[0] <= sums[1] && sums[0] <= sums[2] && acc8 == 0) {
            s32 avoidanceStrength = state->avoidanceActive;
            state->avoidanceTargetOffset = -0x50;
            state->avoidanceStep = -6 - (avoidanceStrength * 2);
        } else if (sums[2] <= sums[1] && sums[2] <= sums[0] && acc9 == 0) {
            s32 avoidanceStrength = state->avoidanceActive;
            state->avoidanceTargetOffset = 0x50;
            state->avoidanceStep = (avoidanceStrength * 2) + 6;
        }
        if (sums[3] >= 0x3E9) {
            s32 fv = state->accelerationLimit;
            s32 d = (fv * 15) << 1;
            state->accelerationLimit = d / 100;
        }
    } else {
        state->avoidanceStep = 0;
        state->avoidanceActive = 0;
        state->avoidanceTargetOffset = state->aiLateralOffset;
    }

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
    s32 *sumPtr;
    s16 indices[4] = {0, 0, 0, 0};

    i = 0;
    sumPtr = sums;
    do {
        *sumPtr = g_Cars[i].progressA + g_Cars[i].progressB;
        i++;
        sumPtr++;
    } while (i < 4);

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


void UpdateRivalRubberBand(void) {
    s32 s6;
    s32 s5;
    s32 s4;
    s32 s3;
    s16 *s2;
    s32 s1;
    s32 s0;

    s6 = g_PlayerCar.progressA + g_PlayerCar.progressB;
    if ((RageSeriesCourseIndex()) == 3) {
        s5 = 0xC00;
        s4 = 0x1400;
    } else {
        s5 = 0x600;
        s4 = 0xE00;
    }

    if (g_RacePhase >= 4) {
        return;
    }

    s1 = 3;
    s3 = 0x20;
    s2 = &g_RivalCueCooldown3;
    s0 = 4;

    do {
        s32 a0 = g_RankedCars[s1]->progressA + g_RankedCars[s1]->progressB - s6;

        if (a0 >= 0) {
            if (s1 == 0) {
                g_RivalCueFlags &= ~1;
            }
            g_ClosestRivalRank = s1;
            if (s4 < a0) {
                g_RivalCueFlags &= ~(0x200 >> s0);
                if (g_RankedCars[s1]->speed >= 0x321) {
                    g_RankedCars[s1]->accelerationLimit = g_RankedCars[s1]->accelerationLimit * 90 / 100;
                }
                return;
            }
            if (s5 < a0) {
                s32 counter;

                if (g_RankedCars[s1]->speed >= 0x3E9) {
                    g_RankedCars[s1]->accelerationLimit = g_RankedCars[s1]->accelerationLimit * 98 / 100;
                }
                counter = *s2;
                g_RivalCueFlags |= (s3 >> s0);
                if (counter >= 0x12D) {
                    *s2 = 0;
                }
                return;
            }
            if (!((0x200 >> s0) & g_RivalCueFlags)) {
                s32 bit;
                s32 flags;
                u32 cueVariant;

                cueVariant = g_SceneTimer;
                switch (cueVariant % 3) {
                default:
                case 0:
                if (g_RivalCueEnabled != 0) {
                    PlaySoundCue(0x32);
                }
                bit = 0x200;
                    break;
                case 1:
                if (g_RivalCueEnabled != 0) {
                    PlaySoundCue(0x33);
                }
                bit = 0x200;
                    break;
                case 2:
                if (g_RivalCueEnabled != 0) {
                    PlaySoundCue(0x34);
                }
                bit = 0x200;
                }

                flags = g_RivalCueFlags;
                *s2 = 0;
                g_RivalCueFlags = (bit >> s0) | flags;
                return;
            }
            {
                RivalCueCooldownAddress counter;
                counter.signedCounter = s2;
                (*counter.unsignedCounter)++;
            }
            return;
        } else {
            if (s1 == 0 && !(g_RivalCueFlags % 2) && a0 < -0x1C00) {
                if (g_RivalCueEnabled != 0 && g_RacePosition == 1) {
                    PlaySoundCue(0x2D);
                }
                g_RivalCueFlags = (g_RivalCueFlags & ~0x10) | 1;
            } else if (a0 >= -0x7FF && !((s3 >> s0) & g_RivalCueFlags)) {
                if (g_SceneTimer % 2) {
                    if (g_RivalCueEnabled != 0) PlaySoundCue(0x2F);
                } else {
                    if (g_RivalCueEnabled != 0) PlaySoundCue(0x30);
                }
                g_RivalCueFlags |= (s3 >> s0);
            } else {
                if (a0 < -0x1000) {
                    g_RivalCueFlags &= ~(s3 >> s0);
                } else if (a0 < -0x800) {
                    if (*s2 >= 0x12D) {
                        if (g_SceneTimer % 2) {
                            if (g_RivalCueEnabled != 0) PlaySoundCue(0x37);
                        } else {
                            if (g_RivalCueEnabled != 0) PlaySoundCue(0x36);
                        }
                        *s2 = 0;
                    }
                }
            }
            s2--;
            s0--;
            s1--;
        }
    } while (s1 >= 0);
}
