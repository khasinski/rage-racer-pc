#include "common.h"
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
    Matrix mScratch1;
    Vec4 vScratch1;
    Vec4 tmp;
    Vec4 vScratch2;
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
    s32 revFlag;
    s32 i;
    s32 cornerIndex;
    u32 skidRange;

    mode23 = g_PadType == 0x23;
    car->facingBackwards = IsCarFacingBackwards(car);

    if (car->drive.manual != 0) {
        if (g_PadPressed & g_PadButtonMapping[4 + mode23 * 8]) {
            s32 g = car->drive.gear;

            if (g < g_CarSpec->topGear && car->drive.clutch == 0) {
                car->drive.gear = car->drive.gear + 1;
                g_SteerHoldFrames = 0;
            }
        }
        if (g_PadPressed & g_PadButtonMapping[5 + mode23 * 8]) {
            s32 g = p->gear;

            if (g >= 2) {
                p->gear = p->gear - 1;
                g_SteerHoldFrames = 0;
            }
        }
    } else {
        if (car->shiftState == 0) {
            s32 g;
            s32 idx;
            s32 tableValue;

            g = car->drive.gear;
            idx = g - 1;
            tableValue = g_CarSpec->shiftPoints[idx].downshiftSpeed;
            if (car->speed < tableValue &&
                g_AutoShiftCooldown <= 0 && car->drive.clutch == 0) {
                if (g >= 2) {
                    car->drive.gear = car->drive.gear - 1;
                    g_AutoShiftCooldown = 25;
                    g_SteerHoldFrames = 0;
                }
            } else {
                GameCarSpec *config;
                GameCarSpecShiftPoint *entry;
                s32 nextGear;
                s32 speed;

                nextGear = p->gear;
                config = g_CarSpec;
                speed = car->speed;
                idx = nextGear - 1;
                entry = config->shiftPoints;
                entry += idx;
                if (entry->upshiftSpeed < speed &&
                    g_AutoShiftCooldown <= 0 && p->clutch == 0 &&
                    nextGear < config->topGear) {
                    p->gear = p->gear + 1;
                    g_AutoShiftCooldown = 25;
                    g_SteerHoldFrames = 0;
                }
            }
        }
        if (g_AutoShiftCooldown > 0) {
            if (p->brakeInput >= 129) {
                g_AutoShiftCooldown = g_AutoShiftCooldown - 2;
            } else {
                g_AutoShiftCooldown = g_AutoShiftCooldown - 1;
            }
        }
        if (car->speed == 0 && p->gear >= 2 && p->motionState != CAR_MOTION_STANDING_START) {
            p->gear = 1;
            p->clutch = 0;
            g_AutoShiftCooldown = 0;
        }
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
        if (g_PadType == 0x41) {
            p->acceleratorInput.sampled =
                ((g_PadHeld & g_PadButtonMapping[2]) != 0) << 8;
            p->brakeInput = ((g_PadHeld & g_PadButtonMapping[3]) != 0) << 8;
        } else if (g_PadType == 0x23) {
            p->acceleratorInput.sampled =
                ((g_PadHeld & g_PadButtonMapping[10]) != 0) << 8;
            p->brakeInput = ((g_PadHeld & g_PadButtonMapping[11]) != 0) << 8;
            switch (g_NegconMappingIndex) {
            case 0:
            case 5:
            {
                CarInputAddress acceleratorInput;

                acceleratorInput.pointer = &p->acceleratorInput.value;
                *acceleratorInput.sampled = (g_NegconAnalogI << 8) / 106;
                p->brakeInput = (g_NegconAnalogII << 8) / 106;
                break;
            }
            case 1:
            case 6:
            {
                CarInputAddress acceleratorInput;

                acceleratorInput.pointer = &p->acceleratorInput.value;
                *acceleratorInput.sampled = (g_NegconAnalogII << 8) / 106;
                p->brakeInput = (g_NegconAnalogI << 8) / 106;
                break;
            }
            case 2:
                p->brakeInput = (g_NegconAnalogL << 8) / 106;
                break;
            case 3:
            {
                CarInputAddress acceleratorInput;

                acceleratorInput.pointer = &p->acceleratorInput.value;
                *acceleratorInput.sampled = (g_NegconAnalogII << 8) / 106;
                p->brakeInput = (g_NegconAnalogL << 8) / 106;
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

    if (g_PadType == 0x23) {
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

    tmp.x = (p->accelPos * 6) / 1280 + car->x + car->motionX;
    tmp.z = (p->brakePos * 6) / 1280 + car->z + car->motionZ;
    SetPlayerPosition(car, &tmp);
    AccumulateLapProgress(GetPlayerCarRuntime(car));

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
        if (limits.rightInset < vout.x) {
            limits.rightKnockbackMode = i;
            limits.rightInset = vout.x;
        } else if (vout.x < limits.leftInset) {
            limits.leftKnockbackMode = i;
            limits.leftInset = vout.x;
        }
    }

    if ((s16)car->motionTimer > 0) {
        ApplyCarKnockback(car);
    }
    skid = UpdateCarTrackState(car, car->trackPointIndex, &limits);
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
    if (skid != 0 || crash != 0) {
        StartCarBodyKick(2, car);
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
            StartCarBodyKick(1, car);
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

    UpdateCarTiltCounter(car);
    UpdateCarCrestHop(car);

    if (skid == 0 && crash == 0) {
        car->y += p->standingStartBounceY;
        UpdateCarBodyKick(car);
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

    if (g_EngineRpm >= g_CarSpec->revLimit - 100 && g_PlayerThrottle >= 129) {
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
                      (0 < g_PlayerThrottle) & (p->clutch == 0) & revFlag);
    } else {
        s32 flag = 0;
        s32 vol = g_EngineRpm + g_EngineRpmJitter;

        if (g_PlayerThrottle > 0) {
            flag = revFlag & 1;
        }
        UpdateLoadedAudioVoices(vol, flag);
    }

    p->gearDisp = p->gear;
}

s32 DrawPlayerTachometer(void) {
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

    return DrawTachometer(g_EngineRpm + g_EngineRpmJitter, g_TachoNeedleFlash, type, amount);
}
