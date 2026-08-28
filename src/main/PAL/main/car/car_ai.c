#include "common.h"
#include "game/car.h"
#include "game/car_internal.h"
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

s32 GetCarCrestTrigger(GameCarRuntime *car) {
    TrackEventData *base;
    s32 pos0;
    s32 pos1;
    s32 row;
    s32 temp;
    s32 crossed;
    s32 i;
    s32 offset;
    s32 sentinel;
    TrackEventDataAddress cursor;
    s32 diff;
    s32 cmp;
    s32 threshold;
    s32 resultOffset;
    TrackEventDataAddress resultCursor;

    base = g_TrackEventData;
    if (car->speed < 0x320) {
        return 0;
    }

    pos0 = car->trackProgress;
    pos1 = car->previousTrackProgress;
    row = car->facingBackwards;

    if (g_RaceSeries != 0) {
        pos0 = g_TrackLength - pos0;
        pos1 = g_TrackLength - pos1;
    }

    if (pos1 < pos0) {
        temp = pos0;
        diff = temp - pos1;
    } else {
    goto not_crossed;

crossed_label:
    crossed = 1;
    goto crest_scan_done;

not_crossed:
    temp = pos1;
    pos1 = pos0;
    diff = temp - pos1;

    }
    if (diff >= 0x1000) {
        temp = 0;
        pos1 = 0;
    }

    crossed = 0;
    i = 0;
    sentinel = -1;
    offset = row * sizeof(TrackCrestEvent[8]);
    cursor.pointer = base;
    cursor.bytePointer += offset;

for (;;) {
    if (!(cursor.pointer->crestEvents[0][0].motionValue == sentinel)) {
    threshold = cursor.pointer->crestEvents[0][0].progress;
    cmp = temp < threshold;
    offset += sizeof(TrackCrestEvent);
    if (!(cmp != 0)) {
    cmp = pos1 < threshold;
    if (cmp != 0) {
        goto crossed_label;
    }

    }
    i++;
    if (i < 8) {
        cursor.pointer = base;
        cursor.bytePointer += offset;
        continue;
    }

    }
break;
}
crest_scan_done:
    if (crossed != 0) {
        resultOffset = i * sizeof(TrackCrestEvent);
        resultOffset += row * sizeof(TrackCrestEvent[8]);
        resultCursor.pointer = base;
        resultCursor.bytePointer += resultOffset;
        return resultCursor.pointer->crestEvents[0][0].motionValue;
    }
    return 0;
}

void UpdateCarCrestHop(GameCarRuntime *car) {
    GameCarRuntime *obj;
    s32 value;
    s32 temp;
    s32 result;
    s32 one;

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

    one = 1;
    obj->verticalMotionState = one;
    if (value > 0) {
        temp = value * obj->speed;
        temp = temp / -4800;
        obj->verticalMotionState = one;
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

void UpdateCarSlideAngle(GameCarRuntime *car, s32 carIndex) {
    GameCarRuntime *obj = car;
    GameCarAiBlock *ai;
    s32 temp;
    s32 value;
    s32 scene;
    u32 slideSign;

    ai = GetCarAiBlock(obj);
    if (obj->slideInput.value == 0) {
        if (!(obj->yawRate != 0)) {
        if (carIndex != 0) {
            value = obj->speed;
            if (value < 0x3C1) {
                return;
            }
            scene = g_RaceSeries;
            carIndex *= value;
            value = carIndex;
            temp = value / 0x320;
            obj->slideInput.value = temp;
            if (scene != 0) {
                temp = -temp;
                obj->slideInput.value = temp;
            }
            obj->yawRate = 0;
            return;
        }
        }
        temp = ai->slideInput.value;
        if (temp == 0) {
            goto slide_input_done;
        }
    }

    value = ai->slideInput.value;
    value = value * 31;
    if (value < 0) {
        value += 0x1F;
    }
    temp = value >> 5;
    slideSign = value;
    value = slideSign >> 31;
    ai->slideInput.value = temp;
    temp += value;
    value = ai->yawRate;
    temp >>= 1;
    value -= temp;
    ai->yawRate = value;
    if (value >= 0x2BC) {
        ai->yawRate = 0x2BC;
        return;
    }
    temp = value < -0x2BB;
    if (temp != 0) {
        temp = -0x2BC;
        ai->yawRate = temp;
        return;
    }
    return;

slide_input_done:
    value = ai->yawRate;
    if (value != 0) {
        temp = value * 15;
        temp <<= 1;
        if (temp < 0) {
            temp += 0x1F;
        }
        temp >>= 5;
        ai->yawRate = temp;
        if (temp == 0) {
            ai->markerDirection = 0;
        }
    }
}

void ApplyCarRacingLineHint(GameCarRuntime *obj, s32 carIndex) {
    GameCarRuntime *objReg = obj;
    s32 target;
    GameCarAiBlock *state;
    s32 index;
    s32 scene;
    TrackRacingLineHint *entry;
    s32 value;
    s32 valueRaw;
    s32 raw;

    raw = objReg->trackProgress;
    
    scene = g_RaceSeries;
    target = raw >> 4;
    index = objReg->routeIndex;
    entry = &g_TrackEventData->racingLineHints[scene][index];

    if (target < 0x20) {
        state = GetCarAiBlock(objReg);
        objReg->routeIndex = 0;
        target = 0;
    } else {
        state = GetCarAiBlock(objReg);
    }

    if (target < entry->start) {
    } else {
    if (entry->end < target) {
        goto advance;
    }
    if (carIndex < 4 && objReg->nearbyCarCount == 0) {
        valueRaw = objReg->aiLateralOffset;
        if (entry->minHeight < valueRaw) {
            value = valueRaw;
            
            raw = entry->maxHeight;
            raw = valueRaw < raw;
            if (raw != 0) {
                raw = value + entry->heightAdjustment;
                objReg->aiLateralOffset = raw;
            }
        }
    }
    return;

    }
    if (!(entry->end < target)) {

    } else {
advance:
    {
        raw = state->routeIndex;
        scene = g_RaceSeries;
        raw++;
        state->routeIndex = raw;
    }
    if (g_TrackEventData->racingLineHints[scene][raw].start == -1) {
        state->routeIndex = 0;
    }
    state->racingLineHintState = 0;
    return;

    }
    objReg->racingLineHintState = 0;
}

void SeedCarRouteMarkers(void) {
    s32 one = 1;
    s32 carIndex;
    s32 scene;
    s32 index;
    s32 target;
    s32 value;

    scene = g_RaceSeries;
    for (carIndex = 0; carIndex < 11; carIndex++) {
        index = 0;
        target = g_Cars[carIndex].trackProgress >> 4;
        g_Cars[carIndex].routeMarkerActive = one;

        while (index < 0x30) {
            value = g_TrackEventData->aiSpeedKeys[scene][index].progress;
            if (target >= value) {
                g_Cars[carIndex].routeMarkerIndex = index;
                break;
            }
            if (value == -1) {
                g_Cars[carIndex].routeMarkerIndex = 0;
                break;
            }
            index++;
        }
    }
}

void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 gear) {
    TrackAiSpeedKey *p[2];
  s16 lim[4];
  s16 val[2];
  GameCarAiBlock *sub_R9;
  s32 rpm;
  s32 g0;
  s32 raw;
  TrackAiSpeedKey *tbl;
  s32 f;
  s32 lo_R7;
  s32 hi;
  s32 range;
  s32 d_R3;
  s32 pitch;
  s32 q;
  s32 cnt;
  s32 one;
  int lowValue;
  raw = car->trackProgress;
  rpm = raw >> 4;
  g0 = car->routeMarkerIndex;
  sub_R9 = GetCarAiBlock(car);
  if (rpm < 0x20)
  {
    car->routeMarkerIndex = 0;
  }
  if (g0 < 0)
  {
    car->routeMarkerIndex = 0;
  }
  tbl = g_TrackEventData->aiSpeedKeys[g_RaceSeries];
  p[0] = &tbl[g0];
  p[1] = &tbl[g0 + 1];
  lim[0] = p[0]->progress;
  lim[1] = p[1]->progress;
  if (gear < 4)
  {
    val[0] = p[0]->targetSpeeds[gear];
    val[1] = p[1]->targetSpeeds[gear];
  }
  else
  {
    f = 0x55 - gear;
    val[0] = (p[0]->targetSpeeds[3] * f) / 100;
    val[1] = (p[1]->targetSpeeds[3] * f) / 100;
  }
  pitch = 0;
  lo_R7 = lim[0];
  if (rpm < lo_R7)
  {
  } else {
  hi = lim[1];
  one = hi;
  if (one < rpm)
  {
    goto L2_inc;
  }
  range = hi - lo_R7;
  pitch = p[0]->pitch;
  if (!(range > 0))
  {
    range = 1;
  }
  d_R3 = rpm - lo_R7;
  lowValue = val[0];
  q = ((val[1] - lowValue) * d_R3) / range;
  sub_R9->accelerationLimit = ((((lowValue + q) * 1168) / 160) * 6) / 100;
  goto target_speed_done;
  }
  if (lim[1] < rpm)
  {
    L2_inc:
    cnt = sub_R9->markerCounter;
    d_R3 = 1;
    q = d_R3;
    g0 = q;
    sub_R9->markerDirection = g0;
    cnt = cnt + 1;
  }
  else
  {
    cnt = sub_R9->markerCounter;
    one = 1;
    sub_R9->markerDirection = one;
    cnt = cnt - 1;
  }

  sub_R9->markerCounter = cnt;
  if (rpm < 0x20)
  {
    sub_R9->markerCounter = 0;
  }
target_speed_done:
  if (sub_R9->markerDirection != 0)
  {
    UpdateCarSlideAngle(car, (s16) pitch);
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
