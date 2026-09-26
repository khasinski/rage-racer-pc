#include "game/driver.h"
#include "game/car_track_internal.h"
#include "game/car_motion_internal.h"
#include "game/random.h"

DriverStep MoveDriver(PlayerCarRuntime *car, const DriverInput *input,
                       const DriverContext *context, u32 *random) {
    const TrackRoute *route = context->route;
    DriveContext drive = context->drive;
    drive.point = RoutePoint(route, car->trackPointIndex);
    drive.nextPoint = RoutePoint(route, WrapSigned32((int64_t)car->trackPointIndex + 1));
    car->facingBackwards = CarFacesBackwards(car, route);
    ApplyDriverInput(car, context->spec, input);
    const int finished = StepCarDynamics(car, context->spec, context->performance,
        &drive, route, context->launchThreshold, random);
    DriverStep step = AdvanceDriver(car, context, random);
    step.motionFinished = finished;
    return step;
}

DriverStep AdvanceDriver(PlayerCarRuntime *car, const DriverContext *context, u32 *random) {
    const TrackRoute *route = context->route;
    DriverStep step = {.skidAngle = -1};
    UpdateCarControlFeedback(car, context->analogSteering);
    IntegratePlayerPosition(car);
    GameCarRuntime *base = AsRivalCar(car);
    const s32 target = FindCarTrackSegment(base, route, car->trackPointIndex);
    MoveCarTrackProgress(base, route, target, context->reverse);
    step.skid = ResolveCarTrackContact(car, route, context->corners, context->reverse);
    StepCarShiftPitch(car, context->spec, random);
    return step;
}

void FinishDriver(PlayerCarRuntime *car, const DriverContext *context,
                   u32 *random, s32 crash, DriverStep *step) {
    const TrackRoute *route = context->route;
    if (step->skid != 0 || crash != 0) {
        const s32 heading = InterpolateRouteAngle(route, car->trackPointIndex,
                                                  car->segmentFraction);
        BeginCarBodyKick(AsRivalCar(car), CAR_BODY_KICK_CORNERING,
                         heading, RandomNext(random));
    }
    CopyPlayerBodyRotationToModel(car);
    car->bodyRoll = WrapSigned32((int64_t)car->bodyRoll + car->bodyRollVelocity);
    car->modelY = car->y;
    const s32 ground = WrapSigned32((int64_t)car->y - CAR_WHEEL_GROUND_OFFSET);
    step->landingFrames = StepPlayerJump(car, context->spec, ground)
        ? car->verticalMotionTimer : 0;
    UpdateCarTilt(car, context->spec, context->drive.started);
    StepCarCrestHop(AsRivalCar(car), context->events, route->length, context->reverse);
    step->skidAngle = ApplyCarContactResponse(car,
        RoutePoint(route, car->trackPointIndex), step->skid, crash);
}
