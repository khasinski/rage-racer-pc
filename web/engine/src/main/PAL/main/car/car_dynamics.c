#include "game/car_drive.h"
#include "game/random.h"

int StepCarMotion(PlayerCarRuntime *car, const GameCarSpec *spec,
                  const TrackRoute *route, const LaunchSpeedThreshold *threshold,
                  u32 *random) {
    int finished = 0;
    switch (car->drive.motionState) {
    case CAR_MOTION_DRIVING:
        StepCarDriving(car, threshold);
        break;
    case CAR_MOTION_TAKEOFF:
        StepCarLaunch(car, spec, route);
        break;
    case CAR_MOTION_AIRBORNE:
        finished = StepCarAirborne(car);
        break;
    case CAR_MOTION_STANDING_START: {
        s32 vertical = 0;
        s32 lateral = 0;
        if (car->drive.standingStartSpin >= CAR_STANDING_START_MIN_SPIN) {
            vertical = RandomNext(random);
            lateral = RandomNext(random);
        }
        finished = StepCarStandingStart(car, vertical, lateral);
        break;
    }
    }
    return finished;
}

int StepCarDynamics(PlayerCarRuntime *car, const GameCarSpec *spec,
                     const CarPerformance *performance,
                     const DriveContext *context, const TrackRoute *route,
                     const LaunchSpeedThreshold *threshold, u32 *random) {
    StepCarDrivetrain(car, spec, performance, context);
    const int finished = context->started
        ? StepCarMotion(car, spec, route, threshold, random) : 0;
    if (car->speed < CAR_STOPPED_SPEED_THRESHOLD) {
        car->headingAngle = car->bodyYaw;
    }
    return finished;
}
