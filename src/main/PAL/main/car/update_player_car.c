#include "game/diagnostics.h"
#include "game/state.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/track_internal.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/random.h"

#include <stdio.h>
#include <stdlib.h>
#include "rage/trace.h"

/*
 * Per-car physics driver (matched sibling of the ASM UpdateAttractCars).
 * Samples input, builds the car's orientation matrices, picks the gear through
 * ShiftPlayerGears, dispatches the engine audio and the boost/launch handlers,
 * and resolves track-boundary skid via UpdateCarTrackState. PlayerCarRuntime
 * and GameCarDrive describe the player layout, whose block at +0xBC is not the
 */
static void UpdatePlayerEngineNote(PlayerCarRuntime *car, GameCarDrive *p) {
    s32 revFlag = 0;

    if (g_EngineRpm >= g_CarSpec->revLimit - 100 &&
        p->acceleratorInput.value >= 129) {
        s32 r = Random15();

        g_TachoNeedleFlash = g_AnimTimer & 2;
        g_EngineRpmJitter = r % 150 / 2;
    } else {
        revFlag = 0;
        if (p->engineRpm == 0 && (g_AnimTimer & 8)) {
            g_TachoNeedleFlash = 0;
            g_EngineRpmJitter = rsin(Random15() & 0xFFF) * 150 / 4096;
            if (g_EngineRpmJitter <= 0) {
                g_EngineRpmJitter = 0;
            }
            revFlag = g_EngineRpmJitter < 37;
        } else {
            g_EngineRpmJitter = 0;
            g_TachoNeedleFlash = 0;
        }
    }

    g_EngineRpmSnapshot = g_EngineRpm;
    if (p->engineRpm != 0) {
        if (p->gear != 1) {
            revFlag = 0;
            if (g_EngineRpm >= g_CarSpec->redline - 2000) {
                revFlag = 1;
                if (g_EngineRpm < g_CarSpec->redline) {
                    revFlag = Random15() & 1;
                }
            }
        } else {
            revFlag = 1;
        }
    }

    if (g_RacePhase >= 4) {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    if (p->manual != 0) {
        UpdateLoadedAudioVoices(g_EngineRpm + g_EngineRpmJitter,
                      (0 < p->acceleratorInput.value) &
                      (p->clutch == 0) & revFlag);
    } else {
        s32 flag = 0;
        s32 vol = g_EngineRpm + g_EngineRpmJitter;

        if (p->acceleratorInput.value > 0) {
            flag = revFlag & 1;
        }
        UpdateLoadedAudioVoices(vol, flag);
    }

    p->gearDisp = p->gear;
    TraceCarMotion("post-update", car);
}

/*
 * How far the steering turns the car this frame. Below walking pace the turn
 * is scaled by the speed itself, so a stopped car does not pivot on the spot,
 * and a car pulling away from the grid is given twice the room.
 */
static void SteerTowardsTarget(PlayerCarRuntime *car, GameCarDrive *p) {
    s32 speed = car->speed;
    s32 turn = (p->steerPos * 6) / 5 * p->steeringGrip;

    if (speed < 256 && p->motionState == CAR_MOTION_DRIVING) {
        p->targetHeading += (turn / 256) * speed / 0x10000;
    } else if (speed < 512 && p->motionState == CAR_MOTION_STANDING_START) {
        p->targetHeading += (turn / 256) * speed / 0x20000;
    } else {
        p->targetHeading += turn / 0x10000;
    }
}

/*
 * The wheels turn with the speed until they would blur, then hold a fixed
 * rate; the top bit asks for the blurred texture.
 */
static void SpinWheels(PlayerCarRuntime *car) {
    s32 step = car->speed * 3;
    s32 spin;

    if (step > 4096) {
        step = 0x249;
    }
    spin = (step + car->wheelRotation) & 0xFFF;
    car->wheelRotation = spin;
    if (car->speed > 800) {
        car->wheelRotation = spin | 0x1000;
    }
}

/*
 * The steering has a stop, and how long it has been held against it drives
 * the wheel-scrub sound. A NeGcon only counts the hold while the twist is
 * still past the stop, and starts its count ten frames in hand; a pad has no
 * travel left to check.
 */
static void ClampSteeringAngle(PlayerCarRuntime *car, GameCarDrive *p) {
    int negcon = g_PadType == 0x23;

    if (car->steeringAngle >= 4096) {
        car->steeringAngle = 4096;
        if (!negcon || p->steerPos < -4096) {
            g_SteerHoldFrames++;
        }
    } else if (car->steeringAngle < -4095) {
        car->steeringAngle = -4096;
        if (!negcon || p->steerPos > 4096) {
            g_SteerHoldFrames++;
        }
    } else {
        g_SteerHoldFrames = negcon ? -10 : 0;
    }
}

/*
 * How far the car reaches across the track, and which corner reaches
 * furthest each way. The corners are turned into the track's frame first, so
 * a car at an angle is measured across its diagonal.
 */
static void MeasureTrackLimits(Matrix *toTrack, CarTrackLimits *limits) {
    SVec corner;
    Vec4 reach;
    s32 index;

    limits->rightInset = -1;
    limits->leftInset = -1;
    for (index = 0; index < 4; index++) {
        corner.vx = g_CarCornerOffsets[index].x * 4;
        corner.vz = g_CarCornerOffsets[index].z * 4;
        corner.vy = 0;
        ApplyMatrix(toTrack, &corner, &reach);
        if (DiagnosticsEnabled("car.track_trace")) {
            const char *timerText = DiagnosticsValue("car.track_trace_timer");
            if (timerText == NULL ||
                g_SceneTimer == (s32)strtol(timerText, NULL, 0)) {
                Trace("car-limit", "timer=%d matrix=%d,%d,%d,%d,%d,%d,%d,%d,%d "
                       "vector=%d,%d,%d output=%d,%d,%d", g_SceneTimer,
                       toTrack->m[0][0], toTrack->m[0][1], toTrack->m[0][2],
                       toTrack->m[1][0], toTrack->m[1][1], toTrack->m[1][2],
                       toTrack->m[2][0], toTrack->m[2][1], toTrack->m[2][2],
                       corner.vx, corner.vy, corner.vz, reach.x, reach.y,
                       reach.z);
            }
        }
        /* The knockback modes are one-based, so that zero means no corner. */
        if (limits->rightInset < reach.x) {
            limits->rightKnockbackMode = index + 1;
            limits->rightInset = reach.x;
        } else if (reach.x < limits->leftInset) {
            limits->leftKnockbackMode = index + 1;
            limits->leftInset = reach.x;
        }
    }
}

/*
 * What a scrape sounds like. Skids one and three are one side of the track and
 * two and four the other, which is why the wall cue is the other way round
 * between the pairs; the first of each pair is a light touch and the second is
 * the one that costs speed. A car nearly straight on scrapes; one at an angle
 * hits the wall.
 */
static void PlaySkidCue(PlayerCarRuntime *car, s32 skid, s32 slip) {
    int lightTouch = (skid == 1) || (skid == 2);
    int nearSide = (skid == 1) || (skid == 3);

    if ((skid < 1) || (skid > 4) || ((s16)car->motionTimer < 15)) {
        return;
    }
    if ((u32)(slip - 768) < 257U) {
        if (lightTouch) {
            PlaySoundCue(0xA);
        } else if (car->speed >= 81) {
            PlaySoundCue(0xD);
        }
        return;
    }
    if (nearSide) {
        PlaySoundCue(g_MirrorMode == 0 ? 0xB : 0xC);
    } else {
        PlaySoundCue(g_MirrorMode == 0 ? 0xC : 0xB);
    }
}

/*
 * The needle chases the engine, twice as fast with the clutch out as with it
 * in, and stops at the limiter and at idle.
 *
 * Retail address 0x8009E808 is not independent storage: it is the +0x78
 * engine-RPM word inside g_PlayerCar.drive. Keeping the symbol as a separate
 * native global leaves it zero and pins the needle to the 500-rpm clamp.
 */
static void SettleEngineRpm(GameCarDrive *p) {
    s32 shown = g_EngineRpm;
    s32 gap = p->engineRpm - shown;
    s32 limit = g_CarSpec->revLimit;

    shown += (p->clutch > 0) ? gap / 2 : gap / 4;
    if (shown >= limit) {
        shown = limit;
    } else if (shown < 500) {
        shown = 500;
    }
    g_EngineRpm = shown;
}

/*
 * A jump. The body rises on one arc and falls on another, both drawn against
 * the tick count since it left the ground, and state two is the pause at the
 * top for a car that has not travelled far enough to start falling yet.
 */
static void UpdateJumpArc(PlayerCarRuntime *car, s32 ground) {
    s32 tick = car->verticalMotionTimer + 1;

    car->verticalMotionTimer = tick;
    if (car->verticalMotionState == 1) {
        s32 rise = (s16)tick;

        car->y = car->verticalMotionRate * rise + (rise * rise * 72) / 100 + car->y;
        if (car->y >= ground) {
            car->verticalMotionState = 0;
        }
    } else if (car->verticalMotionState == 2) {
        if (ground - car->verticalMotionRate <= car->verticalTargetY) {
            car->y = car->verticalTargetY;
        } else {
            car->verticalMotionState = 3;
            car->verticalMotionRate = car->verticalMotionTimer;
            car->y = car->verticalTargetY;
        }
    } else {
        s32 fall = (s16)tick - car->verticalMotionRate;

        car->y = car->verticalTargetY + (fall * fall * 216) / 100;
        if (car->y >= ground) {
            car->verticalMotionState = 0;
        }
    }
}

/*
 * Coming down. The drivetrain has to be restated for the gear the car landed
 * in, because it was turning freely in the air: the torque, the rev target and
 * the load all follow from the speed and the gear rather than from where they
 * had drifted to.
 */
static void RelaunchDrivetrain(PlayerCarRuntime *car, GameCarDrive *p) {
    GameCarSpec *spec = g_CarSpec;
    s32 rpm;

    p->drivetrainTorque = ((100 - (p->gear - 1) * 4) * 10000) * car->speed / 100;
    g_ShiftSoundLevel = car->verticalMotionTimer & 0x3F;
    p->yawOffset = 0;
    p->launchHeading = car->headingAngle;
    p->launchSpeed = car->speed / 0x100000;
    p->spinRate = 0;
    rpm = car->speed * 160 / 1168 * 10000 / spec->gearRatio[p->gear];
    p->jumpTimer = 0x14;
    p->motionState = CAR_MOTION_AIRBORNE;
    g_ShiftTargetRpm = rpm;
    p->shiftRpmDelta = (u16)g_ShiftTargetRpm - (u16)p->engineRpm;
    p->engineLoad = rpm * spec->gearLoad[p->gear] / 0x20000;
    /* An automatic box slips a little, so it asks the engine for less. */
    if (p->manual == 0) {
        p->engineLoad = p->engineLoad * 985 / 1000;
    }
}

/* Back on the ground: the body stops where the wheels are and the suspension
 * takes the impact. A long enough drop lands audibly. */
static void LandFromJump(PlayerCarRuntime *car, GameCarDrive *p, s32 ground) {
    car->y = ground + 8;
    car->verticalPitch = 0;
    car->verticalRoll = 0;
    StartCarBodyKick(1, AsRivalCar(car));
    g_ShiftSoundLevel = 0;
    if ((car->verticalMotionTimer >= 19) && (g_RacePhase < 3)) {
        PlaySoundCue(0xE);
    }
    if ((p->motionState == CAR_MOTION_DRIVING) && (car->verticalMotionTimer >= 3)) {
        RelaunchDrivetrain(car, p);
    }
}

void UpdatePlayerCar(PlayerCarRuntime *car) {
    Matrix m1;
    Matrix m2;
    SVec sv1;
    Vec4 tmp;
    Matrix mA;
    SVec sv2;
    CarTrackLimits limits;
    GameCarDrive *p = &car->drive;
    s32 usesNegconMapping;
    s32 ground;
    s32 slip;
    s32 skid;
    s32 crash;
    s32 bodyY;
    u32 skidRange;

    TraceCarStates();

    usesNegconMapping = g_PadType == PAD_TYPE_NEGCON;
    car->facingBackwards = IsCarFacingBackwards(car);

    ShiftPlayerGears(car, usesNegconMapping);

    UpdateCarBodyRoll(car);

    if (car->verticalMotionState == 0) {
        SteerTowardsTarget(car, p);
    }

    ReadPlayerCarInput(p);
    UpdateCarDrivetrain(car);

    SpinWheels(car);

    ClampSteeringAngle(car, p);

    TraceCarMotion("pre-integrate", car);
    car->x -= car->motionX;
    car->z -= car->motionZ;
    BuildRotMatrixY(&m1, car->bodyYaw);
    BuildRotMatrixX(&m2, car->bodyPitch);
    MulMatrix2(&m2, &m1);
    BuildRotMatrixZ(&m2, car->bodyRoll);
    MulMatrix2(&m2, &m1);

    sv1.vx = 0;
    sv1.vy = 0;
    m2.m[0][0] = m1.m[0][0];
    m2.m[0][1] = m1.m[1][0];
    m2.m[0][2] = m1.m[2][0];
    m2.m[1][0] = m1.m[0][1];
    m2.m[1][1] = m1.m[1][1];
    m2.m[1][2] = m1.m[2][1];
    m2.m[2][0] = m1.m[0][2];
    m2.m[2][1] = m1.m[1][2];
    m2.m[2][2] = m1.m[2][2];
    sv1.vz = -p->bodyLiftOffset - 50;
    ApplyMatrix(&m2, &sv1, &car->motionX);

    /* Retail copied a stack Vec4 after assigning only X/Z. Preserve Y/W
     * explicitly so player state does not depend on the host stack ABI. */
    tmp = GetPlayerPosition(car);
    tmp.x = (p->accelPos * 6) / 1280 + car->x + car->motionX;
    tmp.z = (p->brakePos * 6) / 1280 + car->z + car->motionZ;
    SetPlayerPosition(car, &tmp);
    TraceCarMotion("post-position", car);
    AccumulateLapProgress(GetPlayerCarRuntime(car));
    TraceCarMotion("post-progress", car);

    slip = (car->bodyYaw - 0xC00 +
            TrackPoint(car->trackPointIndex)->angle) & 0xFFF;
    sv2.vx = 0;
    sv2.vz = 0;
    sv2.vy = slip;
    RotMatrix(&sv2, &mA);

    MeasureTrackLimits(&mA, &limits);

    if ((s16)car->motionTimer > 0) {
        ApplyCarKnockback(AsRivalCar(car));
    }
    TraceCarMotion("post-knockback", car);
    skid = UpdateCarTrackState(AsRivalCar(car), car->trackPointIndex, &limits);
    TraceCarMotion("post-track", car);
    skidRange = skid - 2;
    if (skidRange < 2U && car->speed < 64) {
        skid = 0;
    }

    if (p->shiftRpmDelta != 0) {
        s32 d = (g_CarSpec->revLimit + g_CarSpec->redline) / 2 - g_ShiftTargetRpm;
        if (d > 0) {
            car->bodyPitch += (d * Random15()) / 3276700;
        }
    }

    crash = CollidePlayerWithCars(car);
    TraceCarMotion(crash != 0 ? "post-cars-hit" : "post-cars-clear", car);
    if (skid != 0 || crash != 0) {
        StartCarBodyKick(2, AsRivalCar(car));
    }

    bodyY = car->y;
    CopyPlayerBodyRotationToModel(car);
    car->bodyRoll += car->bodyRollVelocity;
    car->modelY = car->y;
    /* Where the wheels sit, eight units under the body. */
    ground = bodyY - 8;

    if (car->verticalMotionState != 0) {
        UpdateJumpArc(car, ground);
        if (car->verticalMotionState == 0) {
            LandFromJump(car, p, ground);
        }
    }

    UpdateCarTiltCounter(AsRivalCar(car));
    UpdateCarCrestHop(AsRivalCar(car));

    if (skid == 0 && crash == 0) {
        car->y += p->standingStartBounceY;
        UpdateCarBodyKick(AsRivalCar(car));
    } else {
        slip = GetAngleDistance(0xC00 - TrackPoint(car->trackPointIndex)->angle,
                             car->headingAngle);
        if (crash != 0) {
            p->launchEnergy -= 1000;
            if (car->speed >= 81) {
                p->drivetrainTorque = p->drivetrainTorque * 98 / 100;
                car->speed = car->speed * 97 / 100;
                p->engineLoad = p->engineLoad * 95 / 100;
                g_ShiftTargetRpm = g_ShiftTargetRpm * 95 / 100;
            }
        } else {
            p->launchEnergy -= 5000;
            p->drivetrainTorque = (85 - rsin(slip) * 20 / 4096) * p->drivetrainTorque / 100;
            car->speed = (87 - rsin(slip) * 40 / 4096) * car->speed / 100;
            p->engineLoad = p->engineLoad * (85 - rsin(slip) * 20 / 4096) / 100;
            g_ShiftTargetRpm = (85 - rsin(slip) * 20 / 4096) * g_ShiftTargetRpm / 100;
            if (g_RacePhase < 3) {
                PlaySkidCue(car, skid, slip);
            }
        }
    }

    SettleEngineRpm(p);

    UpdatePlayerEngineNote(car, p);
}
