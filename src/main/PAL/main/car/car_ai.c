#include "game/car.h"
#include "game/track.h"
#include "game/race.h"
#include "psyq/gte.h"

/*
 * Jump / launch setup: when GetCarCrestTrigger reports a marker crossing, seeds the
 * launch trajectory and snapshots the car's render offsets (bodyPitch/bodyRoll
 * and y). verticalMotionState 1 is the rising jump phase.
 * inline mult/mfhi block is the compiler's divide idiom; keep it verbatim.
 */

void UpdateCarBodyKick(GameCarRuntime *car) {
    s32 value;
    s32 wave;
    s32 amplitude;
    s32 product;

    if (car->motionMode == 0) {
        return;
    }

    car->motionModeTimer--;
    if ((s16)car->motionModeTimer == 0) {
        car->motionMode = 0;
        car->bodyKickOffset = 0;
    }

    {
        s32 timer;

        timer = car->motionModeTimer;
        product = timer * car->motionValue.value;
        if (product < 0) {
            product += 0x7F;
        }
        amplitude = product >> 7;

        wave = rsin(((timer * 3) << 12) / 30) * amplitude;
    }
    if (wave < 0) {
        wave += 0x7FF;
    }
    value = wave >> 11;

    switch (car->motionMode) {
    case 1:
    case 5:
        car->bodyKickOffset = value + amplitude;
        car->bodyPitch += car->bodyKickOffset;
        car->bodyKickOffset = value + amplitude / 2;
        car->bodyRoll += car->bodyKickOffset / 2;
        break;

    case 2:
        if (car->verticalMotionState != 0) {
            break;
        }
        car->bodyRoll += value;
        break;

    case 3:
        car->bodyKickOffset = value + amplitude;
        car->bodyPitch += car->bodyKickOffset;
        break;

    case 4:
        car->bodyRoll += value;
        break;
    }
}

/*
 * The crest the car drove over this frame, if any: the number the track's own
 * table gives that crest, or 0. Each row of the table is up to eight crests
 * for one direction of travel, ordered along the track and ended by a -1.
 */
s32 GetCarCrestTrigger(GameCarRuntime *car) {
    TrackEventData *base;
    s32 low;
    s32 high;
    s32 row;
    s32 i;
    s32 offset;
    TrackEventDataAddress cursor;

    base = g_TrackEventData;
    if (car->speed < 0x320) {
        return 0;
    }

    high = car->trackProgress;
    low = car->previousTrackProgress;
    row = car->facingBackwards;

    if (g_RaceSeries != 0) {
        high = g_TrackLength - high;
        low = g_TrackLength - low;
    }

    /* The stretch covered this frame, low end first. */
    if (low >= high) {
        s32 swap = low;
        low = high;
        high = swap;
    }
    /* A gap that big is the lap wrapping round, not a stretch of track. */
    if (high - low >= 0x1000) {
        low = 0;
        high = 0;
    }

    offset = row * sizeof(TrackCrestEvent[8]);
    for (i = 0; i < 8; i++, offset += sizeof(TrackCrestEvent)) {
        s32 progress;

        cursor.pointer = base;
        cursor.bytePointer += offset;
        if (cursor.pointer->crestEvents[0][0].motionValue == -1) {
            return 0;
        }
        progress = cursor.pointer->crestEvents[0][0].progress;
        if (low < progress && progress <= high) {
            return cursor.pointer->crestEvents[0][0].motionValue;
        }
    }
    return 0;
}

void UpdateCarCrestHop(GameCarRuntime *car) {
    GameCarRuntime *obj;
    s32 value;
    s32 temp;
    s32 result;

    obj = car;

    if (obj->verticalMotionState != 0) {
        result = obj->verticalMotionTimer;
        value = result * result;
        temp = obj->verticalPitch;
        /* /6 is the retail `mult` by 0x2AAAAAAB + `mfhi` - (x >> 31); gcc
         * generates that magic-number sequence for a signed divide by 6. */
        value = value / 6;
        /* These barriers are load-bearing: without them the copy to `result`
         * is scheduled ahead of the divide and the load delay needs a nop. */
        
        result = temp;
        
        if (temp >= 0x12C) {
            value >>= 8;
        }
        result += value;
        obj->verticalPitch = result;

        temp = value;
        if (value < 0) {
            temp = value + 3;
        }
        result = (u16)obj->verticalRoll;
        temp >>= 2;
        result += temp;
        obj->verticalRoll = result;

        result = obj->verticalPitch;
        temp = obj->verticalRoll;
        obj->bodyPitch = result;
        obj->bodyRoll = temp;
        return;
    }

    value = GetCarCrestTrigger(obj);
    if (value == 0) {
        return;
    }

    obj->verticalMotionState = 1;
    if (value > 0) {
        temp = value * obj->speed;
        temp = temp / -4800;
        obj->verticalMotionState = 1;
        obj->verticalMotionRate = temp;
    } else {
        result = 2;
        obj->verticalMotionState = result;
        result = -value;
        obj->verticalMotionRate = result;
    }

    result = (u16)obj->bodyPitch;
    temp = (u16)obj->bodyRoll;
    value = (u16)obj->y;
    obj->verticalMotionTimer = 0;
    obj->verticalPitch = result;
    obj->verticalRoll = temp;
    obj->verticalTargetY = value;
}

/* Ease a slide back towards straight: 15/16 of the yaw rate each frame,
 * rounding towards zero, and drop the marker once it reaches nothing. */
static void SettleSlide(GameCarAiBlock *ai) {
    s32 rate = ai->yawRate;

    if (rate == 0) {
        return;
    }
    rate = rate * 15 * 2;
    if (rate < 0) {
        rate += 0x1F;
    }
    rate >>= 5;
    ai->yawRate = rate;
    if (rate == 0) {
        ai->markerDirection = 0;
    }
}

/*
 * The AI cars' slide: a rival that is not sliding and not turning gets nudged
 * into one in proportion to its speed and its place in the field, and a car
 * that is sliding turns that input into yaw rate, capped either way.
 */
void UpdateCarSlideAngle(GameCarRuntime *car, s32 carIndex) {
    GameCarRuntime *obj = car;
    GameCarAiBlock *ai;
    s32 adjusted;
    s32 input;
    s32 rate;

    ai = GetCarAiBlock(obj);
    if (obj->slideInput.value == 0) {
        if (obj->yawRate == 0 && carIndex != 0) {
            /* Start a slide, away from the racing line in reverse races. */
            if (obj->speed < 0x3C1) {
                return;
            }
            input = (carIndex * obj->speed) / 0x320;
            obj->slideInput.value = g_RaceSeries != 0 ? -input : input;
            obj->yawRate = 0;
            return;
        }
        if (ai->slideInput.value == 0) {
            SettleSlide(ai);
            return;
        }
    }

    /* Decay the input by 31/32, rounding towards zero, and take half of what
     * is left off the yaw rate. The half carries a rounding bit of its own:
     * the sign of the product before the shift, which is set only while that
     * product is still negative after the correction. */
    adjusted = ai->slideInput.value * 31;
    if (adjusted < 0) {
        adjusted += 0x1F;
    }
    input = adjusted >> 5;
    ai->slideInput.value = input;
    rate = ai->yawRate - ((input + (s32)((u32)adjusted >> 31)) >> 1);
    ai->yawRate = rate;
    if (rate >= 0x2BC) {
        ai->yawRate = 0x2BC;
    } else if (rate < -0x2BB) {
        ai->yawRate = -0x2BC;
    }
}

/*
 * Put every car on the speed key it has already reached, at the start of a
 * race. The list is ordered along the track and ends with a -1.
 */
void SeedCarRouteMarkers(void) {
    s32 series = g_RaceSeries;
    s32 carIndex;

    for (carIndex = 0; carIndex < 11; carIndex++) {
        s32 position = g_Cars[carIndex].trackProgress >> 4;
        s32 index;

        g_Cars[carIndex].routeMarkerActive = 1;
        for (index = 0; index < 0x30; index++) {
            s32 progress =
                g_TrackEventData->aiSpeedKeys[series][index].progress;

            if (position >= progress) {
                g_Cars[carIndex].routeMarkerIndex = index;
                break;
            }
            if (progress == -1) {
                g_Cars[carIndex].routeMarkerIndex = 0;
                break;
            }
        }
    }
}

/*
 * The racing line the AI is following: a list of stretches of track, each
 * saying how far off the centre line a car may drift there. A car past the end
 * of its current stretch takes the next one, wrapping at the list's -1.
 *
 * The nudge only applies to the front four and only when nobody is alongside,
 * so cars in traffic keep whatever line the collision code left them on.
 */
void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex) {
    GameCarAiBlock *ai = GetCarAiBlock(car);
    s32 series = g_RaceSeries;
    s32 position = car->trackProgress >> 4;
    TrackRacingLineHint *hint;

    hint = &g_TrackEventData->racingLineHints[series][car->routeIndex];
    /* Before the first stretch of the lap, the list starts over. */
    if (position < 0x20) {
        car->routeIndex = 0;
        position = 0;
    }

    if (hint->end < position) {
        ai->routeIndex++;
        if (g_TrackEventData->racingLineHints[series][ai->routeIndex].start ==
            -1) {
            ai->routeIndex = 0;
        }
        ai->racingLineHintState = 0;
        return;
    }
    if (position < hint->start) {
        car->racingLineHintState = 0;
        return;
    }
    if (carIndex < 4 && car->nearbyCarCount == 0) {
        s32 offset = car->aiLateralOffset;

        if (hint->minHeight < offset && offset < hint->maxHeight) {
            car->aiLateralOffset = offset + hint->heightAdjustment;
        }
    }
}

/*
 * How hard an AI car is allowed to accelerate right now.
 *
 * The track carries a list of keys, each a position along the track and a
 * target speed per gear. A car sitting between two keys gets its limit
 * interpolated between them; a car that has run off either end of its current
 * pair steps its marker towards where it actually is and gets nothing this
 * frame. Above fourth the target is fourth's, scaled down as the gear climbs.
 */
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 gear) {
    TrackAiSpeedKey *keys[2];
    TrackAiSpeedKey *table;
    GameCarAiBlock *ai;
    s32 position;
    s32 marker;
    s32 lowProgress;
    s32 highProgress;
    s32 lowSpeed;
    s32 highSpeed;
    s32 pitch;

    position = car->trackProgress >> 4;
    /*
     * Read before the resets below: retail still indexes with the old marker
     * for this frame, and only the next frame sees the zero.
     *
     * routeMarkerIndex and the AI view's markerCounter are the same halfword,
     * signed here and unsigned there. The signed read is what makes the `< 0`
     * test below mean anything, so both spellings have to stay.
     */
    marker = car->routeMarkerIndex;
    ai = GetCarAiBlock(car);
    if (position < 0x20 || marker < 0) {
        car->routeMarkerIndex = 0;
    }

    table = g_TrackEventData->aiSpeedKeys[g_RaceSeries];
    keys[0] = &table[marker];
    keys[1] = &table[marker + 1];
    lowProgress = keys[0]->progress;
    highProgress = keys[1]->progress;
    if (gear < 4) {
        lowSpeed = keys[0]->targetSpeeds[gear];
        highSpeed = keys[1]->targetSpeeds[gear];
    } else {
        s32 taper = 0x55 - gear;
        lowSpeed = (keys[0]->targetSpeeds[3] * taper) / 100;
        highSpeed = (keys[1]->targetSpeeds[3] * taper) / 100;
    }

    pitch = 0;
    if (position >= lowProgress && position <= highProgress) {
        s32 range = highProgress - lowProgress;
        s32 blended;

        pitch = keys[0]->pitch;
        if (range <= 0) {
            range = 1;
        }
        blended = lowSpeed +
                  (((highSpeed - lowSpeed) * (position - lowProgress)) / range);
        ai->accelerationLimit = (((blended * 1168) / 160) * 6) / 100;
    } else {
        ai->markerDirection = 1;
        ai->markerCounter += (highProgress < position) ? 1 : -1;
        if (position < 0x20) {
            ai->markerCounter = 0;
        }
    }

    if (ai->markerDirection != 0) {
        UpdateCarSlideAngle(car, (s16)pitch);
    }
}

s32 CollideRivalCars(GameCarRuntime *car, s32 index) {
    s16 rotation[4];
    s32 transformed[3];
    Matrix matrix;
    s16 velocityDelta[2];
    CarCollisionPoint quads[4][4];
    CarCollisionPoint samples[5];
    CarCollisionPoint carCorners[4];
    CarCollisionPoint otherCorners[4];
    GameCarRuntime *other;
    s32 nextIndex;
    s32 carProgress;
    s32 carField34;
    s32 hit;
    s32 corner;
    s32 offset;
    s32 quadIndex;
    s32 distance;
    s32 progressDelta;
    s32 average01X;
    s32 average01Z;
    s32 average23X;
    s32 average23Z;
    u32 average02X;
    u32 average02Z;
    u32 average13X;
    u32 average13Z;
    u32 centerX;
    u32 centerZ;
    u32 averageSign;

    other = &g_Cars[index + 1];
    nextIndex = index + 1;
    hit = 0;
    carProgress = car->trackProgress;
    carField34 = car->trackLateralOffset;

    while (nextIndex < 11) {
        if (other->activeFlag != -1 && other->verticalMotionState == car->verticalMotionState) {
            progressDelta =
                (other->trackProgress + g_TrackLength - carProgress) % g_TrackLength;
            distance = other->trackLateralOffset - carField34;
            if (distance < 0) {
                distance = -distance;
            }
            if (distance < 100 &&
                (progressDelta < 200 || g_TrackLength - 200 < progressDelta)) {
                velocityDelta[0] = (u16)other->x - (u16)car->x;
                velocityDelta[1] = (u16)other->z - (u16)car->z;

                rotation[0] = (u16)car->bodyPitch;
                rotation[2] = (u16)car->bodyRoll;
                rotation[1] = (u16)car->bodyYaw;
                RotMatrix(rotation, &matrix);
                SetRotMatrix(&matrix);

                for (corner = 0, offset = 0; corner < 4; corner++, offset++) {
                    rotation[0] = g_CarCollisionCorners[offset].x;
                    rotation[2] = g_CarCollisionCorners[offset].z;
                    rotation[1] = 0;
                    TransformCollisionVector(rotation, transformed);
                    carCorners[offset].x = transformed[0] >> 2;
                    carCorners[offset].z = transformed[2] >> 2;
                    quads[corner][offset] = carCorners[offset];
                }

                average02X = carCorners[0].x + carCorners[2].x;
                average02X += average02X >> 31;
                average02X >>= 1;
                average02Z = carCorners[0].z + carCorners[2].z;
                average02Z += average02Z >> 31;
                average02Z >>= 1;
                average13X = carCorners[1].x + carCorners[3].x;
                average13X += average13X >> 31;
                average13X >>= 1;
                average13Z = carCorners[1].z + carCorners[3].z;
                average13Z += average13Z >> 31;
                average13Z >>= 1;

                average01X = carCorners[0].x + carCorners[1].x;
                averageSign = average01X;
                average01X += averageSign >> 31;
                average01X >>= 1;
                average23X = carCorners[2].x + carCorners[3].x;
                average23X /= 2;
                centerX = (s16)average01X + (s16)average23X;
                centerX += centerX >> 31;
                centerX >>= 1;

                average01Z = carCorners[0].z + carCorners[1].z;
                averageSign = average01Z;
                average01Z += averageSign >> 31;
                average01Z >>= 1;
                average23Z = carCorners[2].z + carCorners[3].z;
                average23Z /= 2;
                centerZ = (s16)average01Z + (s16)average23Z;
                centerZ += centerZ >> 31;
                centerZ >>= 1;

                quads[1][0].x = average01X;
                quads[0][1].x = average01X;
                quads[1][0].z = average01Z;
                quads[0][1].z = average01Z;
                quads[2][0].x = average02X;
                quads[0][2].x = average02X;
                quads[2][0].z = average02Z;
                quads[0][2].z = average02Z;
                quads[3][1].x = average13X;
                quads[1][3].x = average13X;
                quads[3][1].z = average13Z;
                quads[1][3].z = average13Z;
                quads[3][2].x = average23X;
                quads[2][3].x = average23X;
                quads[3][2].z = average23Z;
                quads[2][3].z = average23Z;
                quads[3][0].x = centerX;
                quads[2][1].x = centerX;
                quads[1][2].x = centerX;
                quads[0][3].x = centerX;
                quads[3][0].z = centerZ;
                quads[2][1].z = centerZ;
                quads[1][2].z = centerZ;
                quads[0][3].z = centerZ;

                rotation[0] = (u16)other->bodyPitch;
                rotation[2] = (u16)other->bodyRoll;
                rotation[1] = (u16)other->bodyYaw;
                RotMatrix(rotation, &matrix);
                SetRotMatrix(&matrix);

                for (corner = 0, offset = 0; corner < 4; corner++, offset += 4) {
                    rotation[0] = g_CarCollisionCorners[corner].x;
                    rotation[2] = g_CarCollisionCorners[corner].z;
                    rotation[1] = 0;
                    TransformCollisionVector(rotation, transformed);
                    otherCorners[corner].x =
                        (transformed[0] >> 2) + velocityDelta[0];
                    otherCorners[corner].z =
                        (transformed[2] >> 2) + velocityDelta[1];
                }

                samples[0].x =
                    (otherCorners[0].x + otherCorners[1].x) / 2;
                samples[0].z =
                    (otherCorners[0].z + otherCorners[1].z) / 2;
                samples[1].x =
                    (otherCorners[0].x + otherCorners[2].x) / 2;
                samples[1].z =
                    (otherCorners[0].z + otherCorners[2].z) / 2;
                samples[2].x =
                    (otherCorners[1].x + otherCorners[3].x) / 2;
                samples[2].z =
                    (otherCorners[1].z + otherCorners[3].z) / 2;
                samples[3].x =
                    (otherCorners[2].x + otherCorners[3].x) / 2;
                samples[3].z =
                    (otherCorners[2].z + otherCorners[3].z) / 2;
                samples[4].x =
                    (samples[0].x + samples[2].x) / 2;
                samples[4].z =
                    (samples[0].z + samples[2].z) / 2;

                corner = 0;
                do {
                    quadIndex = 0;
                    do {
                        hit = IsPointInQuad(
                            GetCarCollisionPointPacked(&quads[quadIndex][2]),
                            GetCarCollisionPointPacked(&quads[quadIndex][3]),
                            GetCarCollisionPointPacked(&quads[quadIndex][0]),
                            GetCarCollisionPointPacked(&quads[quadIndex][1]),
                            GetCarCollisionPointPacked(&otherCorners[corner]));
                        if (hit > 0) {
                            hit = quadIndex + 1;
                            break;
                        }
                        quadIndex++;
                    } while (quadIndex < 4);
                    if (hit > 0) {
                        break;
                    }
                    corner++;
                } while (corner < 4);

                if (hit <= 0) {
                    corner = 0;
                    do {
                        quadIndex = 0;
                        do {
                            hit = IsPointInQuad(
                                GetCarCollisionPointPacked(&quads[quadIndex][2]),
                                GetCarCollisionPointPacked(&quads[quadIndex][3]),
                                GetCarCollisionPointPacked(&quads[quadIndex][0]),
                                GetCarCollisionPointPacked(&quads[quadIndex][1]),
                                GetCarCollisionPointPacked(&samples[corner]));
                            if (hit > 0) {
                                hit = quadIndex + 1;
                                break;
                            }
                            quadIndex++;
                        } while (quadIndex < 4);
                        if (hit > 0) {
                            break;
                        }
                        corner++;
                    } while (corner < 5);
                }
                if (hit > 0) {
                    break;
                }
            }
        }
        other++;
        nextIndex++;
    }

    if (hit > 0) {
        if (hit < 3) {
            s32 deltaX;
            s32 deltaZ;

            deltaX =
                (s16)((u16)other->worldVelocityX - (u16)car->worldVelocityX);
            if (deltaX < 0) {
                deltaX += 31;
            }
            velocityDelta[0] = deltaX >> 5;
            deltaZ =
                (s16)((u16)other->worldVelocityZ - (u16)car->worldVelocityZ);
            if (deltaZ < 0) {
                deltaZ += 31;
            }
            velocityDelta[1] = deltaZ >> 5;
            SetCarKnockback(car, 0, 0, 4);
            SetCarKnockback(
                other, velocityDelta[0], velocityDelta[1], 4);
            car->collisionFlag = 1;
            car->acceleration = (car->acceleration * 90) / 100;
            other->collisionFlag = 1;
        } else {
            s32 deltaX;
            s32 deltaZ;

            deltaX =
                (s16)((u16)other->worldVelocityX - (u16)car->worldVelocityX);
            if (deltaX < 0) {
                deltaX += 31;
            }
            velocityDelta[0] = deltaX >> 5;
            deltaZ =
                (s16)((u16)other->worldVelocityZ - (u16)car->worldVelocityZ);
            if (deltaZ < 0) {
                deltaZ += 31;
            }
            velocityDelta[1] = deltaZ >> 5;
            SetCarKnockback(
                car, -velocityDelta[0], -velocityDelta[1], 4);
            SetCarKnockback(other, 0, 0, 4);
            other->acceleration = (other->acceleration * 90) / 100;
            car->collisionFlag = 1;
            other->collisionFlag = 1;
        }
    }
    return hit;
}
