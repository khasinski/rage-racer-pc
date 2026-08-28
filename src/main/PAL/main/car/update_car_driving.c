#include "game/car.h"
#include "game/car_internal.h"
#include "game/race.h"
#include "game/render.h"
#include "game/audio.h"

/*
 * Car motion handler for motionState == CAR_MOTION_DRIVING (normal driving): turns steering into a
 * world velocity, triggers over-rev / redline engine-audio cues (comparing
 * against the spec block's redline at +0x100 / +0x106), advances the car
 * (AdvanceCarPosition), and detects the jump/launch trigger. The drive sub-block is
 * the GameCarDrive view beginning at offset +0xBC.
 */
void UpdateCarDriving(PlayerCarRuntime *car, s32 unused) {
    (void)unused;
    GameCarDrive *route = &car->drive;
    s32 sinA;
    s32 cosA;
    s32 base;
    s32 r;
    s32 coords[3];
    GameCarSpec *spec1;
    GameCarSpec *spec;
    s32 t;
    s32 idx;

    r = GetAngleDelta(car->bodyYaw, route->targetHeading);
    base = car->bodyYaw;
    car->bodyYaw = r / 5 + base;
    AdvanceCarPosition(car, base);

    sinA = rsin(car->bodyYaw);
    cosA = rcos(car->bodyYaw);

    route->accelPos = rsin(car->headingAngle) * car->speed / 256;
    route->brakePos = rcos(car->headingAngle) * car->speed / 256;

    coords[0] = (cosA * route->accelPos - sinA * route->brakePos) / 4096;
    coords[2] = (sinA * route->accelPos + cosA * route->brakePos) / 4096;
    route->accelPos = sinA * coords[2] / 4096;
    route->brakePos = cosA * coords[2] / 4096;

    spec1 = g_CarSpec;
    if (spec1->revLimit + 2000 < route->engineRpm && g_RacePhase >= 2) {
        SetIndexedEffectVoice(0, 0x1800,
                      (route->engineRpm - spec1->revLimit) / 100 + 128);
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    spec = g_CarSpec;
    if (spec->redline + 1000 < route->engineRpm) {
        s16 v = g_SteerHoldFrames;
        if (v >= 41 && route->gear == spec->topGear &&
            car->shiftState == 0) {
            idx = v + 24;
            if (idx >= 101) {
                idx = 100;
            }
            SetIndexedEffectVoice(2, 0x1500, idx);
        } else {
            SetIndexedEffectVoice(-1, 0, 0);
        }
    } else {
        SetIndexedEffectVoice(-1, 0, 0);
    }

    if (route->acceleratorLatch == 1) {
        route->launchEnergy = car->speed * route->groundedFrames;
        route->groundedFrames = 0;
        if (g_LaunchSpeedThresholds[route->launchThresholdIndex].initial < car->speed &&
            route->launchEnergy > route->launchEnergyThreshold) {
            route->motionState = CAR_MOTION_TAKEOFF;
            route->bodyLiftOffset = 0;
            SetIndexedEffectVoice(0, 0, 0);
            t = 1000 - (route->steeringGripResponse - 1000) * 8;
            if (t < 1000) {
                t = 1000;
            }
            route->spinRate = -coords[0] * t / 1000 * 2;
            route->launchDirection = car->facingBackwards;
        }
    } else {
        if (route->acceleratorInput.value < 128) {
            s16 m9e = route->brakeLatch;
            if (m9e == 1) {
                s32 av = coords[0] < 0 ? -coords[0] : coords[0];
                s32 aval = av * car->speed / 64;
                route->launchEnergy = aval;
                if (g_LaunchSpeedThresholds[route->launchThresholdIndex].sustain < car->speed &&
                    route->launchEnergyThreshold < aval) {
                    route->motionState = m9e;
                    route->bodyLiftOffset = 0;
                    SetIndexedEffectVoice(0, 0, 0);
                    route->spinRate = -coords[0];
                    route->launchDirection = car->facingBackwards;
                }
            } else {
                route->groundedFrames++;
                route->launchEnergy = 0;
            }
        } else {
            route->groundedFrames = 0;
            route->launchEnergy = 0;
        }
    }
}
