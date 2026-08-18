#include "common.h"
#include "game/game_input.h"
#include "game/diagnostics.h"
#include "game/vector.h"
#include "psyq/gte.h"
#include "game/state.h"
#include "game/race.h"
#include "game/car.h"
#include "game/input_internal.h"
#include "game/track.h"
#include "game/track_internal.h"
#include "game/render.h"
#include "game/audio.h"
#include "game/random.h"
#include "game/gearbox.h"

#ifdef __psyz
#include <stdio.h>
#include <stdlib.h>

static void RageTraceCarMotion(const char *phase, PlayerCarRuntime *car) {
    static int enabled = -1;
    static int timer = -1;
    if (enabled < 0) {
        const char *text = RageDiagnosticsValue("car.motion_trace_timer");
        enabled = RageDiagnosticsEnabled("car.motion_trace");
        timer = text != NULL ? (int)strtol(text, NULL, 0) : -1;
    }
    if (!enabled || (timer >= 0 && timer != g_SceneTimer)) return;
    printf("car-motion phase=%s timer=%d x=%d z=%d rotation=%d,%d,%d "
           "roll_velocity=%d kick=%d,%d,%d,%d motion=%d,%d "
           "knockback=%d,%d,%d,%d point=%d progress=%d lateral=%d speed=%d\n",
           phase, g_SceneTimer, car->x, car->z, car->bodyYaw, car->bodyPitch,
           car->bodyRoll, car->bodyRollVelocity, car->motionMode,
           car->motionModeTimer, car->motionValue.value, car->bodyKickOffset,
           car->motionX, car->motionZ,
           car->motionActive, car->motionTimer, car->velocityX, car->velocityZ,
           car->trackPointIndex, car->trackProgress, car->trackLateralOffset,
           car->speed);
}

static void RageTraceCarStates(void) {
    static int enabled = -1;
    static int timerMin = -1;
    static int timerMax = -1;
    int index;
    if (enabled < 0) {
        const char *minText = RageDiagnosticsValue("car.state_trace_timer_min");
        const char *maxText = RageDiagnosticsValue("car.state_trace_timer_max");
        enabled = RageDiagnosticsEnabled("car.state_trace");
        timerMin = minText != NULL ? (int)strtol(minText, NULL, 0) : -1;
        timerMax = maxText != NULL ? (int)strtol(maxText, NULL, 0) : -1;
    }
    if (!enabled || (timerMin >= 0 && g_SceneTimer < timerMin) ||
        (timerMax >= 0 && g_SceneTimer > timerMax)) return;
    for (index = 0; index < 11; index++) {
        GameCarRuntime *opponent = &g_Cars[index];
        printf("car-state timer=%d index=%d x=%d z=%d progress=%d lateral=%d "
               "speed=%d point=%d yaw=%d active=%d collision=%d\n",
               g_SceneTimer, index, opponent->x, opponent->z,
               opponent->trackProgress, opponent->trackLateralOffset,
               opponent->speed, opponent->trackPointIndex, opponent->bodyYaw,
               opponent->activeFlag, opponent->collisionFlag);
    }
}
#endif

/*
 * Per-car physics / gear-shift driver (matched sibling of the ASM
 * UpdateAttractCars). Samples input, builds the car's orientation matrices, runs
 * the manual/auto gear-shift state machine (using the per-car spec block
 * g_CarSpec for top-gear/upshift/downshift-speed tables and the shift
 * cooldown timers g_SteerHoldFrames/g_AutoShiftCooldown), dispatches the engine audio and the
 * boost/launch handlers, and resolves track-boundary skid via UpdateCarTrackState.
 * PlayerCarRuntime and GameCarDrive describe the player layout, whose block at
 * +0xBC is not the rival-car GameCarAiBlock view.
 */
void UpdatePlayerCar(PlayerCarRuntime *car) {
    Matrix m1;
    Matrix m2;
    SVec sv1;
    Vec4 tmp;
    Matrix mA;
    SVec sv2;
    Vec4 vout;
    CarTrackLimits limits;
    GameCarDrive *p = &car->drive;
    s32 mode23;
    s32 limit;
    s32 slip;
    s32 skid;
    s32 crash;
    s32 revFlag = 0;
    s32 i;
    s32 cornerIndex;
    u32 skidRange;

#ifdef __psyz
    RageTraceCarStates();
#endif

    mode23 = g_GameInput.controllerType == 0x23;
    car->facingBackwards = IsCarFacingBackwards(car);

    {
        GearboxState gearbox = {
            p->gear, p->clutch, p->manual, p->brakeInput, p->motionState,
            g_AutoShiftCooldown, g_SteerHoldFrames};
        GearboxInput input = {
            car->speed, car->shiftState,
            (g_GameInput.pressed & g_PadButtonMapping[4 + mode23 * 8]) != 0,
            (g_GameInput.pressed & g_PadButtonMapping[5 + mode23 * 8]) != 0,
            g_CarSpec->topGear, g_CarSpec->shiftPoints};
        GearboxUpdate(&gearbox, &input);
        p->gear = gearbox.gear;
        p->clutch = gearbox.clutch;
        g_AutoShiftCooldown = gearbox.autoShiftCooldown;
        g_SteerHoldFrames = gearbox.steerHoldFrames;
    }

    UpdateCarBodyRoll(car);

    if (car->shiftState == 0) {
        s32 spd = car->speed;

        if (spd < 256 && p->motionState == CAR_MOTION_DRIVING) {
            p->targetHeading += ((p->steerPos * 6) / 5 * p->steeringGrip / 256) * spd / 0x10000;
        } else if (spd < 512 && p->motionState == CAR_MOTION_STANDING_START) {
            p->targetHeading += ((p->steerPos * 6) / 5 * p->steeringGrip / 256) * spd / 0x20000;
        } else {
            p->targetHeading += (p->steerPos * 6) / 5 * p->steeringGrip / 0x10000;
        }
    }

    if (g_RacePhase < 4) {
        if (g_GameInput.controllerType == 0x41) {
            p->acceleratorInput.sampled =
                ((g_GameInput.held & g_PadButtonMapping[2]) != 0) << 8;
            p->brakeInput = ((g_GameInput.held & g_PadButtonMapping[3]) != 0) << 8;
        } else if (g_GameInput.controllerType == 0x23) {
            p->acceleratorInput.sampled =
                ((g_GameInput.held & g_PadButtonMapping[10]) != 0) << 8;
            p->brakeInput = ((g_GameInput.held & g_PadButtonMapping[11]) != 0) << 8;
            switch (g_NegconMappingIndex) {
            case 0:
            case 5:
            {
                CarInputAddress acceleratorInput;

                acceleratorInput.pointer = &p->acceleratorInput.value;
                *acceleratorInput.sampled = (g_GameInput.analogI << 8) / 106;
                p->brakeInput = (g_GameInput.analogII << 8) / 106;
                break;
            }
            case 1:
            case 6:
            {
                CarInputAddress acceleratorInput;

                acceleratorInput.pointer = &p->acceleratorInput.value;
                *acceleratorInput.sampled = (g_GameInput.analogII << 8) / 106;
                p->brakeInput = (g_GameInput.analogI << 8) / 106;
                break;
            }
            case 2:
                p->brakeInput = (g_GameInput.analogL << 8) / 106;
                break;
            case 3:
            {
                CarInputAddress acceleratorInput;

                acceleratorInput.pointer = &p->acceleratorInput.value;
                *acceleratorInput.sampled = (g_GameInput.analogII << 8) / 106;
                p->brakeInput = (g_GameInput.analogL << 8) / 106;
                break;
            }
            case 4:
            case 7:
                break;
            }
        } else {
            p->brakeInput = 0;
            p->acceleratorInput.value = 0;
        }
    } else {
        p->acceleratorInput.value = 0;
        p->brakeInput = 0;
    }

    UpdateCarDrivetrain(car);

    {
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

    if (g_GameInput.controllerType == 0x23) {
        if (car->steeringAngle >= 4096) {
            car->steeringAngle = 4096;
            if (p->steerPos < -4096) {
                g_SteerHoldFrames++;
            }
        } else if (car->steeringAngle < -4095) {
            car->steeringAngle = -4096;
            if (p->steerPos > 4096) {
                g_SteerHoldFrames++;
            }
        } else {
            g_SteerHoldFrames = -10;
        }
    } else {
        if (car->steeringAngle >= 4096) {
            car->steeringAngle = 4096;
            g_SteerHoldFrames++;
        } else if (car->steeringAngle < -4095) {
            car->steeringAngle = -4096;
            g_SteerHoldFrames++;
        } else {
            g_SteerHoldFrames = 0;
        }
    }

#ifdef __psyz
    RageTraceCarMotion("pre-integrate", car);
#endif
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
#ifdef __psyz
    RageTraceCarMotion("post-position", car);
#endif
    AccumulateLapProgress(GetPlayerCarRuntime(car));
#ifdef __psyz
    RageTraceCarMotion("post-progress", car);
#endif

    {
        s32 base = car->bodyYaw - 0xC00;

        slip = (base + g_TrackPoints[car->trackPointIndex].angle) & 0xFFF;
    }
    sv2.vx = 0;
    sv2.vz = 0;
    sv2.vy = slip;
    RotMatrix(&sv2, &mA);

    limits.rightInset = 0;
    limits.leftInset = 0;
    limits.rightInset = -1;
    limits.leftInset = -1;
    for (i = 1, cornerIndex = 0; cornerIndex < 4; cornerIndex++, i++) {
        sv2.vx = g_CarCornerOffsets[cornerIndex].x * 4;
        sv2.vz = g_CarCornerOffsets[cornerIndex].z * 4;
        sv2.vy = 0;
        ApplyMatrix(&mA, &sv2, &vout);
#ifdef __psyz
        if (RageDiagnosticsEnabled("car.track_trace")) {
            const char *timerText = RageDiagnosticsValue("car.track_trace_timer");
            if (timerText == NULL || g_SceneTimer == (s32)strtol(timerText, NULL, 0)) {
                printf("car-limit timer=%d matrix=%d,%d,%d,%d,%d,%d,%d,%d,%d "
                       "vector=%d,%d,%d output=%d,%d,%d\n", g_SceneTimer,
                       mA.m[0][0], mA.m[0][1], mA.m[0][2],
                       mA.m[1][0], mA.m[1][1], mA.m[1][2],
                       mA.m[2][0], mA.m[2][1], mA.m[2][2],
                       sv2.vx, sv2.vy, sv2.vz, vout.x, vout.y, vout.z);
            }
        }
#endif
        if (limits.rightInset < vout.x) {
            limits.rightKnockbackMode = i;
            limits.rightInset = vout.x;
        } else if (vout.x < limits.leftInset) {
            limits.leftKnockbackMode = i;
            limits.leftInset = vout.x;
        }
    }

    if ((s16)car->motionTimer > 0) {
        ApplyCarKnockback(GetPlayerCarRuntime(car));
    }
#ifdef __psyz
    RageTraceCarMotion("post-knockback", car);
#endif
    skid = UpdateCarTrackState(GetPlayerCarRuntime(car), car->trackPointIndex, &limits);
#ifdef __psyz
    RageTraceCarMotion("post-track", car);
#endif
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
#ifdef __psyz
    RageTraceCarMotion(crash != 0 ? "post-cars-hit" : "post-cars-clear", car);
#endif
    if (skid != 0 || crash != 0) {
        StartCarBodyKick(2, GetPlayerCarRuntime(car));
    }

    {
        s32 fuel = car->y;

        CopyPlayerBodyRotationToModel(car);
        car->bodyRoll = car->bodyRoll + car->bodyRollVelocity;
        car->modelY = car->y;
        limit = fuel - 8;
    }

    if (car->shiftState != 0) {
        s32 n = car->shiftTick + 1;

        car->shiftTick = n;
        if (car->shiftState == 1) {
            s32 t = (s16)n;

            car->y = car->shiftRef * t + (t * t * 72) / 100 + car->y;
            if (car->y >= limit) {
                car->shiftState = 0;
            }
        } else if (car->shiftState == 2) {
            if (limit - car->shiftRef <= car->shiftBase) {
                car->y = car->shiftBase;
            } else {
                car->shiftState = 3;
                car->shiftRef = car->shiftTick;
                car->y = car->shiftBase;
            }
        } else {
            n = (s16)n - car->shiftRef;

            car->y = car->shiftBase + (n * n * 216) / 100;
            if (car->y >= limit) {
                car->shiftState = 0;
            }
        }

        if (car->shiftState == 0) {
            car->y = limit + 8;
            car->verticalPitch = 0;
            car->verticalRoll = 0;
            StartCarBodyKick(1, GetPlayerCarRuntime(car));
            g_ShiftSoundLevel = 0;
            if ((s16)car->shiftTick >= 19) {
                if (g_RacePhase < 3) {
                    PlaySoundCue(0xE);
                }
            }
            if (p->motionState == CAR_MOTION_DRIVING && (s16)car->shiftTick >= 3) {
                s32 rpm;

                GameCarSpec *props;
                s32 v = (100 - (p->gear - 1) * 4) * 10000;

                p->drivetrainTorque = v * car->speed / 100;
                g_ShiftSoundLevel = car->shiftTick & 0x3F;
                p->yawOffset = 0;
                p->launchHeading = car->headingAngle;
                p->launchSpeed = car->speed / 0x100000;
                p->spinRate = 0;
                props = g_CarSpec;
                {
                    s32 *ratios = props->gearRatio;

                    rpm = car->speed * 160 / 1168 * 10000 / ratios[p->gear];
                }
                p->jumpTimer = 0x14;
                p->motionState = CAR_MOTION_AIRBORNE;
                g_ShiftTargetRpm = rpm;
                p->shiftRpmDelta = (u16)g_ShiftTargetRpm - (u16)p->engineRpm;
                {
                    s32 *loadRow = props->gearLoad;

                    p->engineLoad = rpm * loadRow[p->gear] / 0x20000;
                    if (p->manual == 0) {
                        p->engineLoad = p->engineLoad * 985 / 1000;
                    }
                }
            }
        }
    }

    UpdateCarTiltCounter(GetPlayerCarRuntime(car));
    UpdateCarCrestHop(GetPlayerCarRuntime(car));

    if (skid == 0 && crash == 0) {
        car->y += p->standingStartBounceY;
        UpdateCarBodyKick(GetPlayerCarRuntime(car));
    } else {
        slip = GetAngleDistance(0xC00 - g_TrackPoints[car->trackPointIndex].angle,
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
                switch (skid) {
                case 1:
                case 3:
                    if ((s16)car->motionTimer >= 15) {
                        u32 slipRange = slip - 768;
                        if (slipRange < 257U) {
                            if (skid == 1) {
                                PlaySoundCue(0xA);
                            } else if (car->speed >= 81) {
                                PlaySoundCue(0xD);
                            }
                        } else {
                            PlaySoundCue(g_MirrorMode == 0 ? 0xB : 0xC);
                        }
                    }
                    break;
                case 2:
                case 4:
                    if ((s16)car->motionTimer >= 15) {
                        u32 slipRange = slip - 768;
                        if (slipRange < 257U) {
                            if (skid == 2) {
                                PlaySoundCue(0xA);
                            } else if (car->speed >= 81) {
                                PlaySoundCue(0xD);
                            }
                        } else if (g_MirrorMode == 0) {
                            PlaySoundCue(0xC);
                        } else {
                            PlaySoundCue(0xB);
                        }
                    }
                    break;
                }
            }
        }
    }

    {
        /* Retail address 0x8009E808 is not independent storage: it is the
         * +0x78 engine-RPM word inside g_PlayerCar.drive.  Keeping the symbol
         * as a separate native global leaves it zero and pins the HUD needle
         * to the 500-rpm clamp. */
        s32 d = p->engineRpm;
        s32 cab = g_EngineRpm;
        s32 sum;
        s32 rpmLimit;

        d -= cab;
        if (p->clutch > 0) {
            sum = d / 2 + cab;
        } else {
            sum = d / 4 + cab;
        }
        rpmLimit = g_CarSpec->revLimit;
        g_EngineRpm = sum;
        if (sum >= rpmLimit) {
            g_EngineRpm = rpmLimit;
        } else if (sum < 500) {
            g_EngineRpm = 500;
        }
    }

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
#ifdef __psyz
    RageTraceCarMotion("post-update", car);
#endif
}

void DrawPlayerTachometer(void) {
    s32 value;
    s32 type;
    u32 amount;

    if (g_TrackZoneDark != 3) {
        value = g_EnvScriptClock;
        amount = value - 0x1154;
        if (amount < 0x434C) {
            if (amount < 0x80) {
                type = 3;
            } else {
                amount = value - 0x5420;
                if (amount < 0x80) {
                    type = 1;
                } else {
                    type = 0;
                    amount = 0;
                }
            }
        } else {
            type = 2;
            amount = 0;
        }
    } else {
        type = 2;
        amount = 0;
    }

    DrawTachometer(g_EngineRpm + g_EngineRpmJitter, g_TachoNeedleFlash, type, amount);
}
