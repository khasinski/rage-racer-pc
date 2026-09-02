#include "game/car.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"
#include "rage/trace.h"


/*
 * Car route-steering update. Samples a look-ahead track point (two ahead or two
 * behind depending on the lap-direction flag g_RaceSeries), clamps the lateral
 * offset to the track half-width (`leftHalfWidth`/`rightHalfWidth`), projects the target point
 * off the centre-line along the inward normal (0x1000 - smoothed track angle),
 * then nudges the car's headingAngle toward that target (GetAngleDelta). Writes
 * the steer value into steeringAngle and the rival AI block at `aiEnabled`.
 */
void SteerCarAlongRoute(GameCarRuntime *car) {
    GameCarAiBlock *ai;
    GameTrackPoint *point;
    s32 index;
    s32 offset;
    s32 lateral;
    s32 rem;
    s32 coords[3];
    s32 angle;
    s32 value;
    s32 zValue;
    s32 lowerLimit;
    s32 callArg;

    lateral = car->aiLateralOffset;
    offset = car->trackPointIndex;
    ai = GetCarAiBlock(car);
    car->reservedDC = 0;

    if (ReadStableRaceSeries() != 0) {
        index = offset + 2;
    } else {
        index = offset - 2;
    }

    rem = index;
    if (index < 0) {
        rem = index + g_TrackPointCount;
    }
    index = rem % g_TrackPointCount;

    point = TrackPoint(index);
    if (point->rightHalfWidth < lateral) {
        value = point->rightHalfWidth * car->normalizedLateralOffset;
        lateral = value / 2048;
    } else {
        value = point->leftHalfWidth;
        lowerLimit = -value;
        if (lateral < lowerLimit) {
            value = lowerLimit * car->normalizedLateralOffset;
            lateral = value / 2048;
        }
    }

    InterpolateTrackPoint(index, coords, car->segmentFraction);
    angle = 0x1000 - SmoothTrackAngle(index, car->segmentFraction);

    value = rsin(angle) * lateral;
    if (value < 0) {
        value += 0xFFF;
    }
    coords[0] += value >> 12;

    zValue = rcos(angle) * lateral;
    if (zValue < 0) {
        zValue += 0xFFF;
    }
    coords[2] += zValue >> 12;

    angle = 0x400 - Atan2(coords[0] - car->x, coords[2] - car->z);

    callArg = ReadStableRaceSeries();
    value = car->trackHeading.value;
    callArg = (callArg << 11) + 0xC00;
    value = -GetAngleDelta(callArg - value, angle);
    car->steeringAngle = value * 3;

    if (car->verticalMotionState == 0) {
        value = GetAngleDelta(car->headingAngle, angle);
        value += car->headingAngle;
        car->headingAngle = value;
        ai->targetYaw = value;
        car->bodyYaw = value;
    }
}

/*
 * Runs the rival-car update passes used by an interactive race. Cars 4..10
 * split their traffic-avoidance work across alternating frames.
 */
/* Every car starts the frame from the heading it settled on last frame, and
 * keeps only the low bit of whatever it was touching. */
static void StartCarFrames(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        car->reservedF8 = 0;
        car->bodyYaw = car->baseBodyYaw;
        car->collisionFlag = (u16)car->collisionFlag & 1;
    }
}

/*
 * Looking for a way past the car in front. Only the leading four do it every
 * frame; the rest take turns, odd cars on odd frames, which halves the work
 * without anyone noticing at the back of the field.
 */
static void AvoidTrafficThisFrame(void) {
    s16 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        s32 slot = (s16)index;

        if (slot >= 4 && (index & 1) != (g_AnimTimer & 1)) {
            continue;
        }
        if (g_Cars[slot].activeFlag != -1) {
            UpdateCarTrafficAvoidance(&g_Cars[slot], slot);
        }
    }
}

/*
 * How hard each car pulls this frame, and how far it swings towards where it
 * wants to be pointing. A car on a boost gets the boost's own acceleration
 * until it is already quick enough, and its own otherwise; the speed keeps a
 * little under two thirds of itself each frame, so the acceleration is what
 * holds it up.
 */
static void AccelerateAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        GameCarAiBlock *ai = GetCarAiBlock(car);

        if (car->activeFlag != -1) {
            if (car->boostTimer > 0) {
                if (car->boostAccelerationThreshold < car->boostTimer &&
                    car->speed >= 0x321) {
                    car->acceleration = 0;
                } else if (ai->accelerationLimit >= car->acceleration) {
                    car->acceleration = ai->boostAcceleration + car->acceleration;
                } else {
                    car->acceleration = ai->accelerationLimit;
                }
                ai->boostTimer--;
            } else if (car->accelerationLimit >= car->acceleration) {
                car->acceleration = car->accelerationStep + car->acceleration;
            } else {
                car->acceleration = car->accelerationLimit;
            }
            car->speed = car->speed * 0x5E / 100;
            car->speed = car->speed + car->acceleration;
            car->bodyYaw =
                GetAngleDelta(car->bodyYaw, ai->targetYaw) / 5 + car->bodyYaw;
        }
    }
}

/*
 * A rival's wheels, turning with the speed until they would blur, then at a
 * fixed rate; the top bit asks for the blurred texture. The same rule the
 * player's car uses.
 */
static void SpinCarWheels(GameCarRuntime *car) {
    s32 scaled = car->speed * 3;
    s16 step = scaled;
    s32 spin;

    if ((s16)scaled >= 0x1001) {
        step = 0x249;
    }
    spin = (step + car->wheelRotation) & 0xFFF;
    car->wheelRotation = spin;
    if (car->speed >= 0x321) {
        car->wheelRotation = spin | 0x1000;
    }
}

/*
 * A rival in the air. It rises on one arc and falls on another, both drawn
 * against the tick count since it left the ground, and state two is the pause
 * at the top for a car that has not travelled far enough to fall yet. The
 * player's car does the same thing under different field names.
 */
static void UpdateCarJumpArc(GameCarRuntime *car, s32 ground) {
    s32 tick = (u16)car->verticalMotionTimer + 1;
    s32 state;

    car->verticalMotionTimer = tick;
    state = car->verticalMotionState;
    if (state == 1) {
        s32 rise = (s16)tick;

        car->y = car->verticalMotionRate * rise + rise * rise * 72 / 100 + car->y;
        if (car->y >= ground) {
            car->verticalMotionState = 0;
        }
    } else if (state == 2) {
        if (car->verticalTargetY >= ground - car->verticalMotionRate) {
            car->y = car->verticalTargetY;
        } else {
            car->verticalMotionState = 3;
            car->verticalMotionRate = car->verticalMotionTimer;
            car->y = car->verticalTargetY;
        }
    } else {
        s16 fall = tick - (u16)car->verticalMotionRate;

        car->y = car->verticalTargetY + fall * fall * 216 / 100;
        if (car->y >= ground) {
            car->verticalMotionState = 0;
        }
    }
    if (car->verticalMotionState == 0) {
        car->y = ground + 8;
        car->verticalPitch = 0;
        car->verticalRoll = 0;
        StartCarBodyKick(1, car);
    }
}

/*
 * What is left of a rival's frame once it has been steered and moved: the
 * wheels, the body following the chassis, the jump if it is in one, and either
 * the suspension settling or the speed lost to whatever it just hit.
 */
static void SettleAllCarBodies(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        if (car->activeFlag != -1) {
            /* Where the wheels sit, eight units under the body. */
            s32 ground = car->y - 8;

            SpinCarWheels(car);
            CopyCarBodyRotationToModel(car);
            car->bodyRoll = car->bodyRoll + car->bodyRollVelocity;
            car->modelY = car->y;
            if (car->verticalMotionState != 0) {
                UpdateCarJumpArc(car, ground);
            }
            if (car->collisionFlag == 0) {
                UpdateCarBodyKick(car);
                UpdateCarCrestHop(car);
            } else {
                car->speed = car->speed * 97 / 100 * 97 / 100;
            }
        }
    }
}

/* Each car against the ones behind it, so every pair is tested once. */
static void CollideAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT - 1; index++) {
        CollideRivalCars(&g_Cars[index], index);
    }
}

/* Where each car wants to be on the road, and how it gets there. */
static void SteerAllCars(void) {
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];

        UpdateCarAiTargetSpeed(car, index);
        ApplyCarRacingLineHint(car, index);
        ClampCarLateralOffset(car, index);
        SteerCarAlongRoute(car);
    }
}

/*
 * How far round each car is, and where the track has put it. A rival's hull is
 * a fixed width here rather than measured from its corners, which is what the
 * player's car does.
 */
static void PlaceAllCarsOnTrack(void) {
    CarTrackLimits limits;
    s16 index;

    limits.rightInset = 0x3C;
    limits.leftInset = -0x3C;
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[(s16)index];

        if (car->activeFlag != -1) {
            AccumulateLapProgress(car);
        }
    }
    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[(s16)index];

        if (car->activeFlag != -1) {
            if ((s16)car->motionTimer > 0) {
                ApplyCarKnockback(car);
            }
            UpdateCarTrackState(car, car->trackPointIndex, &limits);
        }
    }
}

/*
 * Where every car ends up. The heading gives the world velocity, and the four
 * cars nearest the camera also get their body lean worked out properly: the
 * chassis is rotated into place, the lean is taken out along its own axis and
 * put back, so a car leaning into a corner does not slide sideways doing it.
 * The rest of the field skips that, because nobody can see it.
 */
static void MoveAllCars(void) {
    Matrix bodyRotation;
    Matrix work;
    SVec lean;
    s32 index;

    for (index = 0; index < RACE_CAR_SLOT_COUNT; index++) {
        GameCarRuntime *car = &g_Cars[index];
        GameCarAiBlock *ai = GetCarAiBlock(car);

        if (car->activeFlag != -1) {
            s32 scaled;

            car->baseBodyYaw = car->bodyYaw;
            scaled = rsin(car->headingAngle) * car->speed;
            if (scaled < 0) {
                scaled += 0xFF;
            }
            car->worldVelocityX = scaled >> 8;
            scaled = rcos(car->headingAngle) * car->speed;
            if (scaled < 0) {
                scaled += 0xFF;
            }
            car->worldVelocityZ = scaled >> 8;
            if (index < 4) {
                s32 yawStep = car->yawRate;
                s32 sixth = (yawStep < 0) ? -yawStep / 6 : yawStep / 6;

                car->x = car->x - car->motionX;
                car->z = car->z - car->motionZ;
                BuildRotMatrixY(&bodyRotation, car->bodyYaw);
                BuildRotMatrixX(&work, car->bodyPitch);
                MulMatrix2(&work, &bodyRotation);
                BuildRotMatrixZ(&work, car->bodyRoll);
                MulMatrix2(&work, &bodyRotation);
                lean.vx = 0;
                lean.vy = 0;
                lean.vz = -sixth - 0x32;
                /* The transpose, which turns the rotation back the other way. */
                work.m[0][0] = bodyRotation.m[0][0];
                work.m[0][1] = bodyRotation.m[1][0];
                work.m[0][2] = bodyRotation.m[2][0];
                work.m[1][0] = bodyRotation.m[0][1];
                work.m[1][1] = bodyRotation.m[1][1];
                work.m[1][2] = bodyRotation.m[2][1];
                work.m[2][0] = bodyRotation.m[0][2];
                work.m[2][1] = bodyRotation.m[1][2];
                work.m[2][2] = bodyRotation.m[2][2];
                ApplyMatrix(&work, &lean, &car->motionX);
                car->x = car->x + car->motionX;
                car->z = car->z + car->motionZ;
            }
            car->x += ai->worldVelocityX * 6 / 1280;
            car->z += ai->worldVelocityZ * 6 / 1280;
            /* The body leans away from the steering and rights itself. */
            if (car->steeringAngle >= 0x41) {
                car->bodyRollVelocity = car->bodyRollVelocity - 6;
            } else if (car->steeringAngle < -0x40) {
                car->bodyRollVelocity = car->bodyRollVelocity + 6;
            }
            if (car->bodyRollVelocity != 0) {
                car->bodyRollVelocity = car->bodyRollVelocity * 7 / 8;
            }
            car->steeringAngle = car->steeringAngle + ai->yawRate;
            if (car->steeringAngle >= 0x12C) {
                car->steeringAngle = 0x12C;
            } else if (car->steeringAngle < -0x12B) {
                car->steeringAngle = -0x12C;
            }
            car->bodyYaw = car->bodyYaw + ai->yawRate;
        }
    }
}

/* Everyone ahead of the player is held back a little, nearest one first. */
static void SlowTheCarsAhead(void) {
    s16 rank = (s16)g_ClosestRivalRank;

    while ((s16)rank > 0) {
        s32 slot = (s16)rank;

        SlowRivalAhead(g_RankedCars[slot], slot);
        rank--;
    }
}

void UpdateRaceCars(void) {
    StartCarFrames();
    RankContenders();
    AvoidTrafficThisFrame();
    CollideAllCars();
    SteerAllCars();
    UpdateRivalRubberBand();
    SlowTheCarsAhead();
    AccelerateAllCars();
    MoveAllCars();
    PlaceAllCarsOnTrack();
    SettleAllCarBodies();
}

static void AccelerateAttractCars(void) {
    s32 i;

    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        GameCarRuntime *car = &g_Cars[i];
        GameCarAiBlock *ai = GetCarAiBlock(car);

        if (car->activeFlag == -1) {
            continue;
        }
        if (car->acceleration < car->accelerationLimit) {
            car->acceleration += car->accelerationStep;
        } else {
            car->acceleration = car->accelerationLimit;
        }
        car->speed = car->speed * 94 / 100 + car->acceleration;
        car->bodyYaw += GetAngleDelta(car->bodyYaw, ai->targetYaw) / 5;
    }
}

/* Runs the corresponding all-cars pass for attract and replay scenes. */
void UpdateAttractCars(void) {
    s32 i;

    TraceCarStates();
    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        GameCarRuntime *car = &g_Cars[i];

        car->reservedF8 = 0;
        car->collisionFlag = 0;
        car->bodyYaw = car->baseBodyYaw;
        car->progressA %= g_TrackLength;
    }
    for (i = 0; i < RACE_CAR_SLOT_COUNT; i++) {
        if (g_Cars[i].activeFlag != -1) {
            UpdateCarTrafficAvoidance(&g_Cars[i], i);
        }
    }
    CollideAllCars();
    SteerAllCars();
    AccelerateAttractCars();
    MoveAllCars();
    PlaceAllCarsOnTrack();
    SettleAllCarBodies();
}
