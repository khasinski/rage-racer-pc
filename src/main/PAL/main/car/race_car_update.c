#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/state.h"
#include "game/track.h"


/*
 * Car route-steering update. Samples a look-ahead track point (two ahead or two
 * behind depending on the lap-direction flag g_RaceSeries), clamps the lateral
 * offset to the track half-width (`leftHalfWidth`/`rightHalfWidth`), projects the target point
 * off the centre-line along the inward normal (0x1000 - smoothed track angle),
 * then nudges the car's headingAngle toward that target (GetAngleDelta). Writes
 * the steer value into steeringAngle and the rival AI block at `aiEnabled`.
 * Register-pinned locals are match-load-bearing.
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
    GameCarRuntime *car = g_Cars;
    s16 index = 0;

    do {
        car->reservedF8 = 0;
        car->bodyYaw = car->baseBodyYaw;
        car->collisionFlag = (u16)car->collisionFlag & 1;
        index++;
        car++;
    } while ((s16)index < 11);
}

/*
 * Looking for a way past the car in front. Only the leading four do it every
 * frame; the rest take turns, odd cars on odd frames, which halves the work
 * without anyone noticing at the back of the field.
 */
static void AvoidTrafficThisFrame(void) {
    s16 index;

    for (index = 0; index < 11; index++) {
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
    GameCarRuntime *car = g_Cars;
    s16 index = 0;

    do {
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
        index++;
        car++;
    } while ((s16)index < 11);
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
        car->verticalMotionState = 0;
        StartCarBodyKick(1, car);
    }
}

/*
 * What is left of a rival's frame once it has been steered and moved: the
 * wheels, the body following the chassis, the jump if it is in one, and either
 * the suspension settling or the speed lost to whatever it just hit.
 */
static void SettleAllCarBodies(void) {
    GameCarRuntime *car = g_Cars;
    s16 index = 0;

    do {
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
        index++;
        car++;
    } while (index < 11);
}

/* Each car against the ones behind it, so every pair is tested once. */
static void CollideAllCars(void) {
    GameCarRuntime *car = g_Cars;
    s16 index = 0;

    do {
        CollideRivalCars(car, (s16)index);
        index++;
        car++;
    } while ((s16)index < 10);
}

/* Where each car wants to be on the road, and how it gets there. */
static void SteerAllCars(void) {
    GameCarRuntime *car = g_Cars;
    s16 index = 0;

    do {
        s32 slot = (s16)index;

        UpdateCarAiTargetSpeed(car, slot);
        ApplyCarRacingLineHint(car, slot);
        ClampCarLateralOffset(car, slot);
        SteerCarAlongRoute(car);
        index++;
        car++;
    } while ((s16)index < 11);
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
    for (index = 0; index < 11; index++) {
        GameCarRuntime *car = &g_Cars[(s16)index];

        if (car->activeFlag != -1) {
            AccumulateLapProgress(car);
        }
    }
    for (index = 0; index < 11; index++) {
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
    Vec4 position;
    Matrix bodyRotation;
    Matrix work;
    SVec lean;
    GameCarRuntime *car = g_Cars;
    s16 index = 0;

    do {
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
            if ((s16)index < 4) {
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
            /*
             * Retail copied an otherwise uninitialized stack Vec4 after
             * assigning only x and z. Its MIPS stack happened to leave zero
             * in positionW, while a 64-bit host does not. Preserve the two
             * untouched coordinates explicitly: positionW is later consumed
             * by camera/render paths, so retaining the undefined copy makes
             * game behaviour depend on the compiler ABI.
             */
            position = *GetCarVector4(car);
            position.x = ai->worldVelocityX * 6 / 1280 + car->x;
            position.z = ai->worldVelocityZ * 6 / 1280 + car->z;
            *GetCarVector4(car) = position;
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
        index++;
        car++;
    } while ((s16)index < 11);
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

/* Runs the corresponding all-cars pass for attract and replay scenes. */
void UpdateAttractCars(void) {
    Vec4 vTmp;
    /* See UpdateRaceCars: these two Matrix workspaces shape retail's frame. */
    CarTrackLimits limits;
    Matrix m1;
    Matrix m2;
    SVec sv1;
    /* These pins reproduce the retail induction registers. */
    GameCarRuntime *car;
    GameCarRuntime *sub;
    s16 i;
    GameCarRuntime *c0;
    c0 = g_Cars;
    for (i = 0; i < 11; i++) {
        c0->reservedF8 = 0;
        c0->collisionFlag = 0;
        c0->bodyYaw = c0->baseBodyYaw;
        c0->progressA = ((c0->progressA) % (g_TrackLength));
        c0++;
    }
    for (i = 0; i < 11; i++) {
        if (((g_Cars[(s16)i]).activeFlag != -1)) {
            UpdateCarTrafficAvoidance(&g_Cars[(s16)i], (s16)i);
        }
    }
    i = 0;
    car = g_Cars;
    do {
        CollideRivalCars(car, (s16)i);
        i++;
        car++;
    } while (i < 10);
    i = 0;
    car = g_Cars;
    do {
        UpdateCarAiTargetSpeed(car, (s16)i);
        ApplyCarRacingLineHint(car, (s16)i);
        ClampCarLateralOffset(car, (s16)i);
        SteerCarAlongRoute(car);
        i++;
        car++;
    } while (i < 11);
    {
        GameCarAiBlock *drive;
        i = 0;
        car = g_Cars;
        sub = g_Cars;
        do {
        if (sub->activeFlag != -1) {
            drive = GetCarAiBlock(car);

            if (sub->acceleration < sub->accelerationLimit) {
                sub->acceleration = sub->accelerationStep + sub->acceleration;
            } else {
                sub->acceleration = sub->accelerationLimit;
            }
            sub->speed = sub->speed * 94 / 100;
            sub->speed = sub->speed + sub->acceleration;
            sub->bodyYaw =
                GetAngleDelta(sub->bodyYaw, drive->targetYaw) / 5 + sub->bodyYaw;
        }
        i++;
        sub++;
        car++;
        } while (i < 11);
        {
        GameCarRuntime *base;
        i = 0;
        car = g_Cars;
        base = g_Cars;
        do {
        drive = GetCarAiBlock(car);

        if (base->activeFlag != -1) {
            s32 t;
            base->baseBodyYaw = base->bodyYaw;
            t = rsin(base->headingAngle) * base->speed;
            if (t < 0) {
                t += 0xFF;
            }
            base->worldVelocityX = t >> 8;
            base->worldVelocityZ = rcos(base->headingAngle) * base->speed / 256;
            if ((s16)i < 4) {
                s32 sixth;
                s32 yawStep;
                car->x = car->x - base->motionX;
                yawStep = base->yawRate;
                base->z = base->z - base->motionZ;
                if (yawStep < 0) {
                    sixth = -yawStep / 6;
                } else {
                    sixth = yawStep / 6;
                }
                BuildRotMatrixY(&m1, base->bodyYaw);
                BuildRotMatrixX(&m2, base->bodyPitch);
                MulMatrix2(&m2, &m1);
                BuildRotMatrixZ(&m2, base->bodyRoll);
                MulMatrix2(&m2, &m1);
                sv1.vx = 0;
                sv1.vy = 0;
                sv1.vz = -sixth - 0x32;
                m2.m[0][0] = m1.m[0][0];
                m2.m[0][1] = m1.m[1][0];
                m2.m[0][2] = m1.m[2][0];
                m2.m[1][0] = m1.m[0][1];
                m2.m[1][1] = m1.m[1][1];
                m2.m[1][2] = m1.m[2][1];
                m2.m[2][0] = m1.m[0][2];
                m2.m[2][1] = m1.m[1][2];
                m2.m[2][2] = m1.m[2][2];
                ApplyMatrix(&m2, &sv1, &car->motionX);
                car->x = car->x + base->motionX;
                base->z = base->z + base->motionZ;
            }
            /* Preserve the vertical coordinate and fourth word while moving
             * the attract car in X/Z.  Copying an otherwise uninitialized
             * stack Vec4 happened to work in the retail executable, but is
             * undefined and makes the result depend on the host ABI. */
            vTmp = *GetCarVector4(car);
            vTmp.x = drive->worldVelocityX * 6 / 1280 + car->x;
            vTmp.z = drive->worldVelocityZ * 6 / 1280 + base->z;
            *GetCarVector4(car) = vTmp;
            if (base->steeringAngle >= 0x41) {
                base->bodyRollVelocity = base->bodyRollVelocity - 6;
            } else if (base->steeringAngle < -0x40) {
                base->bodyRollVelocity = base->bodyRollVelocity + 6;
            }
            if (base->bodyRollVelocity != 0) {
                base->bodyRollVelocity = base->bodyRollVelocity * 7 / 8;
            }
            base->steeringAngle = base->steeringAngle + drive->yawRate;
            if (base->steeringAngle >= 0x12C) {
                base->steeringAngle = 0x12C;
            } else if (base->steeringAngle < -0x12B) {
                base->steeringAngle = -0x12C;
            }
            base->bodyYaw = base->bodyYaw + drive->yawRate;
        }
        i++;
        base++;
        car++;
        } while (i < 11);
        }
    }
    limits.rightInset = 0x3C;
    limits.leftInset = -0x3C;
    for (i = 0; i < 11; i++) {
        if (((g_Cars[(s16)i]).activeFlag != -1)) {
            AccumulateLapProgress(&g_Cars[(s16)i]);
        }
    }
    for (i = 0; i < 11; i++) {
        if (((g_Cars[(s16)i]).activeFlag != -1)) {
            if ((s16)g_Cars[(s16)i].motionTimer > 0) {
                ApplyCarKnockback(&g_Cars[(s16)i]);
            }
            UpdateCarTrackState(
                &g_Cars[(s16)i],
                g_Cars[(s16)i].trackPointIndex,
                &limits);
        }
    }
    {
    GameCarRuntime *base;
    i = 0;
    car = g_Cars;
    base = g_Cars;
    do {
        if (base->activeFlag != -1) {
            s16 step;
            s32 spin;
            s32 scaled;
            s32 limit;
            scaled = base->speed * 3;
            step = scaled;
            if ((s16)scaled >= 0x1001) {
                step = 0x249;
            }
            spin = (step + base->wheelRotation) & 0xFFF;
            base->wheelRotation = spin;
            if (base->speed >= 0x321) {
                base->wheelRotation = spin | 0x1000;
            }
            limit = base->y - 8;
            CopyCarBodyRotationToModel(base);
            base->bodyRoll = base->bodyRoll + base->bodyRollVelocity;
            base->modelY = base->y;
            if (base->verticalMotionState != 0) {
                s32 tick;
                s32 state;
                tick = (u16)base->verticalMotionTimer + 1;
                base->verticalMotionTimer = tick;
                state = base->verticalMotionState;
                if (state == 1) {
                    s32 t = (s16)tick;
                    base->y =
                        base->verticalMotionRate * t + t * t * 72 / 100 + base->y;
                    if (base->y >= limit) {
                        base->verticalMotionState = 0;
                    }
                } else if (state == 2) {
                    if (base->verticalTargetY >= limit - base->verticalMotionRate) {
                        base->y = base->verticalTargetY;
                    } else {
                        base->verticalMotionState = 3;
                        base->verticalMotionRate = base->verticalMotionTimer;
                        base->y = base->verticalTargetY;
                    }
                } else {
                    s16 n = tick - (u16)base->verticalMotionRate;
                    base->y = base->verticalTargetY + n * n * 216 / 100;
                    if (base->y >= limit) {
                        base->verticalMotionState = 0;
                    }
                }
                if (base->verticalMotionState == 0) {
                    base->y = limit + 8;
                    base->verticalPitch = 0;
                    base->verticalRoll = 0;
                    base->verticalMotionState = 0;
                    StartCarBodyKick(1, car);
                }
            }
            if (base->collisionFlag == 0) {
                UpdateCarBodyKick(car);
                UpdateCarCrestHop(car);
            } else {
                base->speed = base->speed * 97 / 100 * 97 / 100;
            }
        }
        i++;
        base++;
        car++;
    } while (i < 11);
    }
}

void RunRaceIntroCamera(PlayerCarRuntime *car, s32 mode) {
    PlayerCarPositionView target;
    ScratchLegacyViewWords legacyView;
    s32 *spad;
    /* The barrier this replaced carried the value in its operand. */
    s32 s0v = 28;
    s32 delta[3];

    LoadScratchLegacyView(&legacyView);
    spad = legacyView.words;
    target.car = car;
    
    if (mode < 90) {
        if (mode < 2) {
            RaceIntroCameraScript *script = g_RaceIntroCameraScript;
            s16 n = script->firstKeyIndex[ReadStableRaceSeries()];
            RaceIntroCameraKey *p;
            RaceIntroCameraKey *q;
            p = &script->keys[n];
            g_RaceIntroCameraCursor = p;
            SCRATCH_VIEW_X = p->x.word;
            SCRATCH_VIEW_Y = p->y.word;
            SCRATCH_VIEW_Z = p->z.word;
            g_RageScratchpadState.reserved14 = p->mode;
            q = g_RaceIntroCameraCursor;
            g_RaceIntroCameraDelta.vx = -q[0].x.half.value + q[1].x.half.value;
            g_RaceIntroCameraDelta.vy = -q[0].y.half.value + q[1].y.half.value;
            g_RaceIntroCameraDelta.vz = -q[0].z.half.value + q[1].z.half.value;
            g_RaceIntroCameraTimer = q[0].duration;
        } else {
            RaceIntroCameraKey *a = g_RaceIntroCameraCursor;
            if (mode == a->startFrame) {
                g_RaceIntroCameraCursor = &a[1];
                g_RaceIntroCameraTimer = a[1].duration;
                if (a[1].mode == 1) {
                    g_RaceIntroCameraDelta.vx = -a[1].x.half.value + target.position->x.half.low;
                    g_RaceIntroCameraDelta.vy = -a[1].y.half.value - 28 + target.position->y.half.low;
                    g_RaceIntroCameraDelta.vz = -a[1].z.half.value + target.position->z.half.low;
                } else {
                    g_RaceIntroCameraDelta.vx = -a[1].x.half.value + a[2].x.half.value;
                    g_RaceIntroCameraDelta.vy = -a[1].y.half.value + a[2].y.half.value;
                    g_RaceIntroCameraDelta.vz = -a[1].z.half.value + a[2].z.half.value;
                }
            }
        }

        g_RaceIntroCameraTimer--;
        if (g_RaceIntroCameraTimer <= 0) {
            g_RaceIntroCameraTimer = 0;
        }

        if (g_RaceIntroCameraCursor->mode == 0) {
            spad[2] = g_RaceIntroCameraCursor->x.word
                      + (g_RaceIntroCameraDelta.vx * rcos((g_RaceIntroCameraTimer << 10) / g_RaceIntroCameraCursor->duration)) / 4096;
            spad[3] = g_RaceIntroCameraCursor->y.word
                      + (g_RaceIntroCameraDelta.vy * rcos((g_RaceIntroCameraTimer << 10) / g_RaceIntroCameraCursor->duration)) / 4096;
            spad[4] = g_RaceIntroCameraCursor->z.word
                      + (g_RaceIntroCameraDelta.vz * rcos((g_RaceIntroCameraTimer << 10) / g_RaceIntroCameraCursor->duration)) / 4096;

            delta[0] = rsin(car->bodyYaw) / 128 + car->x - spad[2];
            delta[1] = car->y - s0v - spad[3];
            delta[2] = rcos(car->bodyYaw) / 128 + car->z - spad[4];
            s0v = 0x400;
            spad[7] = s0v - Atan2(delta[0], delta[2]);
            s0v = s0v - Atan2(delta[1], DistanceXZ(delta[0], delta[2]) >> 6);
            spad[6] = s0v;
            spad[8] = 0;
            StoreScratchLegacyView(&legacyView);
            SetCameraRotMatrix();
            SelectModelBank(0);
            DrawPlayerCarModel((GameRenderObject *)car);
        } else {
            DrawFullscreenFadeTile(g_RaceIntroCameraTimer * 26, 0x29);
            {
                s32 c0 = car->x;
                s32 c1 = car->y;
                s32 c2 = car->z;
                s32 c3 = car->positionW;
                spad[2] = c0;
                spad[3] = c1;
                spad[4] = c2;
                spad[5] = c3;
            }
            
            spad[3] -= s0v;
            {
                s32 c0 = car->bodyPitch;
                s32 c1 = car->bodyYaw;
                s32 c2 = car->bodyRoll;
                s32 c3 = car->bodyRotationW;
                spad[6] = c0;
                spad[7] = c1;
                spad[8] = c2;
                spad[9] = c3;
            }
            
            StoreScratchLegacyView(&legacyView);
            SetCameraRotMatrix();
        }
    } else {
        UpdateCamera(CAMERA_VIEW_CAR, (GameRenderObject *)car);
    }
}

void SeedFinishCamera(PlayerCarRuntime *car) {
    u32 word0;
    Block16 *src;
    Block16 *dst;
    Block16 *end;
    GameCarRuntimeAddress sourceAddress;
    GameTrackPoint *track;
    GameCarRuntimeAddress destinationAddress;
    GameTrackPoint *point;
    s32 index;
    s32 lastIndex;

    sourceAddress.player = car;
    destinationAddress.runtime = &g_CameraCar;
    dst = destinationAddress.blocks;
    src = sourceAddress.blocks;
    end = src + sizeof(GameCarRuntime) / sizeof(*src);
    do {
        *dst = *src;
        src++;
        dst++;
    } while (src != end);

    sourceAddress.blocks = src;
    destinationAddress.blocks = dst;
    *destinationAddress.vector = *sourceAddress.vector;

    index = car->trackPointIndex;
    track = g_TrackPoints;
    point = &track[index];
    g_CameraCar.x = point->x;

    index = car->trackPointIndex;
    point = &track[index];
    g_CameraCar.z = point->z;

    index = car->trackPointIndex;
    point = &track[index];
    index = g_CameraCar.speed;
    word0 = point->y;
    index += 0x40;
    word0 -= 0x40;
    g_CameraCar.speed = index;
    g_CameraCar.y = word0;

    index = car->facingBackwards;
    lastIndex = car->trackPointIndex;
    index <<= 11;
    point = &track[lastIndex];
    index += 0xC00;
    index -= point->angle;
    g_CameraCar.headingAngle = index;
    g_CameraCarSeedYaw = index;
    g_CameraCar.bodyYaw = index;
}
