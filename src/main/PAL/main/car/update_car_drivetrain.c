#include "game/car_internal.h"
#include "game/race.h"
#include "game/state.h"
#include "game/audio.h"
#include "game/random.h"

static void PlayMotionVoice(const PlayerCarRuntime *car) {
    switch (car->drive.motionState) {
    case CAR_MOTION_DRIVING:
        PlayCarDrivingVoice(car, g_CarSpec);
        break;
    case CAR_MOTION_TAKEOFF:
        PlayCarLaunchVoice(car);
        break;
    case CAR_MOTION_AIRBORNE:
        PlayCarAirborneVoice(car);
        break;
    case CAR_MOTION_STANDING_START:
        PlayCarStandingStartVoice(car);
        break;
    }
}

void UpdateCarDrivetrain(PlayerCarRuntime *car) {
    DriveContext context = {
        .racing = g_RacePhase == RACE_PHASE_ACTIVE,
        .digitalSteering = g_PadType == PAD_TYPE_DIGITAL,
        .started = g_RacePhase >= RACE_PHASE_ACTIVE,
    };
    if (g_TrackPoints != NULL && g_TrackPointCount > 0) {
        context.point = TrackPoint(car->trackPointIndex);
        context.nextPoint = TrackPoint(car->trackPointIndex + 1);
    }
    StepCarDrivetrain(car, g_CarSpec, &g_CarPerformance, &context);
    if (context.started) {
        const s32 motion = car->drive.motionState;
        PlayMotionVoice(car);
        const TrackRoute route = {.points = g_TrackPoints, .count = g_TrackPointCount};
        const LaunchSpeedThreshold *threshold = &g_LaunchSpeedThresholds[
            NormalizeCarLaunchThresholdIndex(car->drive.launchThresholdIndex)];
        const int finished = StepCarMotion(car, g_CarSpec, &route, threshold, &g_RandomSeed);
        if (motion == CAR_MOTION_DRIVING && car->drive.motionState == CAR_MOTION_TAKEOFF) {
            SetIndexedEffectVoice(0, 0, 0);
        }
        if (finished) SetIndexedEffectVoice(-1, 0, 0);
    }
    if (car->speed < CAR_STOPPED_SPEED_THRESHOLD) {
        car->headingAngle = car->bodyYaw;
    }
}
