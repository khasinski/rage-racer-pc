#include "common.h"
#include "game/asset.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/render_internal.h"
#include "game/render_workspace.h"
#include "game/scratchpad_legacy.h"
#include "game/state.h"
#include "game/track.h"
#include "game/vector.h"
#include "psyq/gte.h"


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
    register s32 value asm("$2");
    s32 zValue;
    s32 lowerLimit;
    register s32 callArg asm("$4");

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

    point = &g_TrackPoints[index];
    if (point->rightHalfWidth < lateral) {
        value = point->rightHalfWidth * car->normalizedLateralOffset;
        if (value < 0) {
            value += 0x7FF;
        }
        lateral = value >> 11;
    } else {
        value = point->leftHalfWidth;
        lowerLimit = -value;
        if (lateral < lowerLimit) {
            value = lowerLimit * car->normalizedLateralOffset;
            if (value < 0) {
                value += 0x7FF;
            }
            lateral = value >> 11;
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
void UpdateRaceCars(void) {
    Vec4 vpos;
    /*
     * GCC 2.6.3 keeps these two Matrix-sized source workspaces in the debug
     * frame even though their high-level scratch values are optimized away.
     */
    CarTrackLimits limits;
    Matrix m1;
    Matrix m2;
    SVec sv;
    /* These pins reproduce the retail induction and matrix registers. */
    register GameCarRuntime *base asm("$18");
    register GameCarAiBlock *drive;
    register Matrix *pm1;
    register Matrix *pm2;
    s16 i;
    GameCarRuntime *q;
    s32 t;
    i = 0;
    q = g_Cars;
    do {
        q->reservedF8 = 0;
        q->bodyYaw = q->baseBodyYaw;
        q->collisionFlag = (u16)q->collisionFlag & 1;
        i++;
        q++;
    } while ((s16)i < 11);
    RankContenders();
    for (i = 0; i < 11; i++) {
        s32 j = (s16)i;
        if (j >= 4 && (i & 1) != (g_AnimTimer & 1)) {
            continue;
        }
        if (g_Cars[j].activeFlag != -1) {
            UpdateCarTrafficAvoidance(&g_Cars[j], j);
        }
    }
    i = 0;
    base = g_Cars;
    do {
        CollideRivalCars(base, (s16)i);
        i++;
        base++;
    } while ((s16)i < 10);
    i = 0;
    base = g_Cars;
    do {
        s32 j = (s16)i;
        UpdateCarAiTargetSpeed(base, j);
        ApplyCarRacingLineHint(base, j);
        ClampCarLateralOffset(base, j);
        SteerCarAlongRoute(base);
        i++;
        base++;
    } while ((s16)i < 11);
    UpdateRivalRubberBand();
    i = (s16)g_ClosestRivalRank;
    if (i > 0) {
        do {
            s32 j = (s16)i;
            SlowRivalAhead(g_RankedCars[j], j);
            i--;
        } while ((s16)i > 0);
    }
    {
    GameCarRuntime *walk;
    i = 0;
    base = g_Cars;
    walk = g_Cars;
    do {
        if (walk->activeFlag != -1) {
            drive = GetCarAiBlock(base);
            if (walk->boostTimer > 0) {
                if (walk->boostAccelerationThreshold < walk->boostTimer && walk->speed >= 0x321) {
                    walk->acceleration = 0;
                } else if (drive->accelerationLimit >= walk->acceleration) {
                    walk->acceleration = drive->boostAcceleration + walk->acceleration;
                } else {
                    walk->acceleration = drive->accelerationLimit;
                }
                drive->boostTimer = drive->boostTimer - 1;
            } else if (walk->accelerationLimit >= walk->acceleration) {
                walk->acceleration = walk->accelerationStep + walk->acceleration;
            } else {
                walk->acceleration = walk->accelerationLimit;
            }
            walk->speed = walk->speed * 0x5E / 100;
            walk->speed = walk->speed + walk->acceleration;
            walk->bodyYaw =
                GetAngleDelta(walk->bodyYaw, drive->targetYaw) / 5 + walk->bodyYaw;
        }
        i++;
        walk++;
        base++;
    } while ((s16)i < 11);
    }
    {
    GameCarRuntime *walk;
    i = 0;
    base = g_Cars;
    pm1 = &m1;
    pm2 = &m2;
    walk = g_Cars;
    do {
        drive = GetCarAiBlock(base);
        if (walk->activeFlag != -1) {
            walk->baseBodyYaw = walk->bodyYaw;
            t = rsin(walk->headingAngle) * walk->speed;
            if (t < 0) {
                t += 0xFF;
            }
            walk->worldVelocityX = t >> 8;
            t = rcos(walk->headingAngle) * walk->speed;
            if (t < 0) {
                t += 0xFF;
            }
            walk->worldVelocityZ = t >> 8;
            if ((s16)i < 4) {
                s32 sixth;
                s32 yawStep;
                base->x = base->x - walk->motionX;
                yawStep = walk->yawRate;
                walk->z = walk->z - walk->motionZ;
                if (yawStep < 0) {
                    sixth = -yawStep / 6;
                } else {
                    sixth = yawStep / 6;
                }
                BuildRotMatrixY(pm1, walk->bodyYaw);
                BuildRotMatrixX(pm2, walk->bodyPitch);
                MulMatrix2(pm2, pm1);
                BuildRotMatrixZ(pm2, walk->bodyRoll);
                MulMatrix2(pm2, pm1);
                sv.vx = 0;
                sv.vy = 0;
                sv.vz = -sixth - 0x32;
                m2.m[0][0] = m1.m[0][0];
                m2.m[0][1] = m1.m[1][0];
                m2.m[0][2] = m1.m[2][0];
                m2.m[1][0] = m1.m[0][1];
                m2.m[1][1] = m1.m[1][1];
                m2.m[1][2] = m1.m[2][1];
                m2.m[2][0] = m1.m[0][2];
                m2.m[2][1] = m1.m[1][2];
                m2.m[2][2] = m1.m[2][2];
                ApplyMatrix(pm2, &sv, &base->motionX);
                base->x = base->x + walk->motionX;
                walk->z = walk->z + walk->motionZ;
            }
            /*
             * Retail copied an otherwise uninitialized stack Vec4 after
             * assigning only x and z. Its MIPS stack happened to leave zero
             * in positionW, while a 64-bit host does not. Preserve the two
             * untouched coordinates explicitly: positionW is later consumed
             * by camera/render paths, so retaining the undefined copy makes
             * game behaviour depend on the compiler ABI.
             */
            vpos = *GetCarVector4(base);
            vpos.x = drive->worldVelocityX * 6 / 1280 + base->x;
            vpos.z = drive->worldVelocityZ * 6 / 1280 + walk->z;
            *GetCarVector4(base) = vpos;
            if (walk->steeringAngle >= 0x41) {
                walk->bodyRollVelocity = walk->bodyRollVelocity - 6;
            } else if (walk->steeringAngle < -0x40) {
                walk->bodyRollVelocity = walk->bodyRollVelocity + 6;
            }
            if (walk->bodyRollVelocity != 0) {
                walk->bodyRollVelocity = walk->bodyRollVelocity * 7 / 8;
            }
            walk->steeringAngle = walk->steeringAngle + drive->yawRate;
            if (walk->steeringAngle >= 0x12C) {
                walk->steeringAngle = 0x12C;
            } else if (walk->steeringAngle < -0x12B) {
                walk->steeringAngle = -0x12C;
            }
            walk->bodyYaw = walk->bodyYaw + drive->yawRate;
        }
        i++;
        walk++;
        base++;
    } while ((s16)i < 11);
    }
    limits.rightInset = 0x3C;
    limits.leftInset = -0x3C;
    for (i = 0; i < 11; i++) {
        if (g_Cars[(s16)i].activeFlag != -1) {
            AccumulateLapProgress(&g_Cars[(s16)i]);
        }
    }
    for (i = 0; i < 11; i++) {
        if (g_Cars[(s16)i].activeFlag != -1) {
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
        GameCarRuntime *lastBase;
        i = 0;
        base = g_Cars;
        lastBase = g_Cars;
        do {
        if (lastBase->activeFlag != -1) {
            register s16 step asm("$17");
            register s32 spin asm("$2");
            s32 scaled;
            s32 limit;
            scaled = lastBase->speed * 3;
            step = scaled;
            if ((s16)scaled >= 0x1001) {
                step = 0x249;
            }
            spin = (step + lastBase->wheelRotation) & 0xFFF;
            lastBase->wheelRotation = spin;
            if (lastBase->speed >= 0x321) {
                lastBase->wheelRotation = spin | 0x1000;
            }
            limit = lastBase->y - 8;
            CopyCarBodyRotationToModel(lastBase);
            lastBase->bodyRoll = lastBase->bodyRoll + lastBase->bodyRollVelocity;
            lastBase->modelY = lastBase->y;
            if (lastBase->verticalMotionState != 0) {
                s32 tick;
                s32 state;
                tick = (u16)lastBase->verticalMotionTimer + 1;
                lastBase->verticalMotionTimer = tick;
                state = lastBase->verticalMotionState;
                if (state == 1) {
                    s32 n = (s16)tick;
                    lastBase->y =
                        lastBase->verticalMotionRate * n + n * n * 72 / 100
                        + lastBase->y;
                    if (lastBase->y >= limit) {
                        lastBase->verticalMotionState = 0;
                    }
                } else if (state == 2) {
                    if (lastBase->verticalTargetY >= limit - lastBase->verticalMotionRate) {
                        lastBase->y = lastBase->verticalTargetY;
                    } else {
                        lastBase->verticalMotionState = 3;
                        lastBase->verticalMotionRate = lastBase->verticalMotionTimer;
                        lastBase->y = lastBase->verticalTargetY;
                    }
                } else {
                    s16 n = tick - (u16)lastBase->verticalMotionRate;
                    lastBase->y = lastBase->verticalTargetY + n * n * 216 / 100;
                    if (lastBase->y >= limit) {
                        lastBase->verticalMotionState = 0;
                    }
                }
                if (lastBase->verticalMotionState == 0) {
                    lastBase->y = limit + 8;
                    lastBase->verticalPitch = 0;
                    lastBase->verticalRoll = 0;
                    lastBase->verticalMotionState = 0;
                    StartCarBodyKick(1, base);
                }
            }
            if (lastBase->collisionFlag == 0) {
                UpdateCarBodyKick(base);
                UpdateCarCrestHop(base);
            } else {
                lastBase->speed = lastBase->speed * 97 / 100 * 97 / 100;
            }
        }
        i++;
        lastBase++;
        base++;
        } while (i < 11);
    }
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
    register GameCarRuntime *car;
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
        register GameCarAiBlock *drive;
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
            register s16 step asm("$17");
            register s32 spin asm("$2");
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
    register s32 s0v asm("$16");
    s32 delta[3];

    LoadScratchLegacyView(&legacyView);
    spad = legacyView.words;
    target.car = car;
    __asm__("" : "=r"(s0v) : "0"(28), "r"(spad));
    if (mode < 90) {
        if (mode < 2) {
            RaceIntroCameraScript *script = g_RaceIntroCameraScript;
            s16 n = script->firstKeyIndex[ReadStableRaceSeries()];
            RaceIntroCameraKey *p;
            RaceIntroCameraKey *q;
            p = &script->keys[n];
            g_RaceIntroCameraCursor = p;
            RENDER_VIEW_X = p->x.word;
            RENDER_VIEW_Y = p->y.word;
            RENDER_VIEW_Z = p->z.word;
            g_RenderWorkspace.reserved14 = p->mode;
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

        g_RaceIntroCameraTimer = g_RaceIntroCameraTimer - 1;
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
            s0v = s0v - Atan2(delta[1], SquareRoot12(delta[0] * delta[0] + delta[2] * delta[2]) >> 6);
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
            __asm__ volatile("");
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
            __asm__ volatile("");
            StoreScratchLegacyView(&legacyView);
            SetCameraRotMatrix();
        }
    } else {
        UpdateCamera(CAMERA_VIEW_CAR, (GameRenderObject *)car);
    }
}

void SeedFinishCamera(PlayerCarRuntime *car) {
    register u32 word0;
    Block16 *src;
    Block16 *dst;
    Block16 *end;
    GameCarRuntimeAddress sourceAddress;
    GameTrackPoint *track;
    GameCarRuntimeAddress destinationAddress;
    GameTrackPoint *point;
    register s32 index asm("$3");
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
