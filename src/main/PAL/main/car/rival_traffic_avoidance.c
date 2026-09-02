#include "game/player_car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/track.h"

/* Rival traffic scanning and lateral avoidance decisions. */

/*
 * Slot 11 holds the player; the rivals fill 0 to 10. Outside the race scene
 * the player is not on the road to be avoided, so the walk stops before it.
 */
enum {
    TRAFFIC_PLAYER_SLOT = RACE_CAR_SLOT_COUNT,
    TRAFFIC_SLOT_COUNT = RACE_CAR_SLOT_COUNT + 1,
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

static void ResetTrafficAvoidance(GameCarRuntime *car) {
    car->avoidanceActive = 0;
    car->avoidanceStep = 0;
    car->avoidanceTargetOffset = car->aiLateralOffset;
}

/* These fields were recovered through an unsigned halfword overlay. Keep its
 * wrap explicitly while the rest of the AI uses the canonical car layout. */
static void AdvanceTrafficLateralOffset(GameCarRuntime *car) {
    car->aiLateralOffset =
        (s16)((u16)car->aiLateralOffset + (u16)car->avoidanceStep);
}

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

    if (trackLength <= 0 || carIndex < 0 ||
        carIndex >= RACE_CAR_SLOT_COUNT) {
        ResetTrafficAvoidance(car);
        car->nearbyCarCount = 0;
        return;
    }

    lane[0] = lane[1] = lane[2] = 0;
    car->avoidanceStep = 0;
    car->nearbyCarCount = 0;

    for (slot = 0; slot < TRAFFIC_SLOT_COUNT; slot++) {
        s32 otherOffset;
        s32 otherSpeed;
        s32 otherProgress;
        s32 alreadyAvoiding;
        /* How far ahead to look for this particular car. */
        s32 lookahead = ownLookahead;
        s32 sideways;
        s32 gap;

        if (g_SceneId != TRAFFIC_RACE_SCENE && slot == TRAFFIC_PLAYER_SLOT) {
            break;
        }
        if (slot == carIndex) {
            continue;
        }

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
            if (other->activeFlag == -1) {
                continue;
            }
            otherProgress = other->trackProgress + trackLength;
            otherOffset = other->trackLateralOffset;
            otherSpeed = (u16)other->speed;
            alreadyAvoiding = (u16)car->avoidanceActive;
        }

        sideways = otherOffset - lateralOffset;
        gap = (otherProgress - progress) % trackLength;

        if (gap <= 0 || gap >= lookahead) {
            /* Behind, but not far behind. */
            if (behindLine < gap) {
                car->nearbyCarCount++;
            }
            continue;
        }

        car->nearbyCarCount++;

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
                car->avoidanceActive = 1;
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
            if ((s16)sideways >= TRAFFIC_SHOULDER + 1) {
                blockingRight += 0xC00 - gap;
            } else if ((s16)sideways < -TRAFFIC_SHOULDER) {
                blockingLeft += 0xC00 - gap;
            }
        }
    }

    crowding = lane[0] + lane[1] + lane[2];
    if (crowding <= 0) {
        ResetTrafficAvoidance(car);
        AdvanceTrafficLateralOffset(car);
        return;
    }

    /*
     * Already out towards one edge with nothing on the other side is reason
     * enough to cross, and it is crossed harder than a choice made on the
     * buckets alone. Otherwise the emptiest side wins, if it is free.
     */
    urgency = car->avoidanceActive;
    if (blockingLeft == 0 && lateralOffset >= TRAFFIC_TARGET_OFFSET + 1) {
        car->avoidanceTargetOffset = -TRAFFIC_TARGET_OFFSET;
        car->avoidanceStep = -8 - urgency * 2;
    } else if (blockingRight == 0 &&
               lateralOffset < -TRAFFIC_TARGET_OFFSET) {
        car->avoidanceTargetOffset = TRAFFIC_TARGET_OFFSET;
        car->avoidanceStep = 8 + urgency * 2;
    } else if (lane[0] <= lane[1] && lane[0] <= lane[2] &&
               blockingLeft == 0) {
        car->avoidanceTargetOffset = -TRAFFIC_TARGET_OFFSET;
        car->avoidanceStep = -6 - urgency * 2;
    } else if (lane[2] <= lane[1] && lane[2] <= lane[0] &&
               blockingRight == 0) {
        car->avoidanceTargetOffset = TRAFFIC_TARGET_OFFSET;
        car->avoidanceStep = 6 + urgency * 2;
    }

    /* Boxed in badly enough, and the car is not allowed to press on as hard.
     * Thirty hundredths, written as fifteen doubled. */
    if (crowding >= 0x3E9) {
        car->accelerationLimit =
            (s16)(car->accelerationLimit * 30 / 100);
    }

    AdvanceTrafficLateralOffset(car);
}
