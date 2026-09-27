#include "game/driver.h"
#include "game/car_track_internal.h"
#include "game/car_motion_internal.h"
#include <string.h>

static void ResetDriver(PlayerCarRuntime *car, const DriverStart *start) {

    memset(car, 0, sizeof(*car));
    car->modelIndex = start->modelIndex;
    car->drive.manual = start->manual;
    car->drive.launchThresholdIndex = start->launchThresholdIndex;
    car->drive.hudLapHighlightRow = -1;
    car->drive.motionState = CAR_MOTION_STANDING_START;
    car->drive.drivetrainCoupled = 1;
    car->drive.dragScale = 1000;
    car->drive.gear = CAR_FIRST_FORWARD_GEAR;
    car->drive.racePosition = 1;
    car->drive.gearDisp = CAR_FIRST_FORWARD_GEAR;
}

static void PlaceDriver(PlayerCarRuntime *car, const DriverStart *setup) {
    CarTrackLimits trackLimits = {0};
    const TrackRivalStart *start;
    const TrackRoute *route = setup->route;
    s32 raceSeries = setup->reverse;
    s32 startPointIndex;

    if (setup->position == NULL || route == NULL || route->points == NULL ||
        route->count <= 0 || route->length <= 0) {
        return;
    }

    start = setup->position;
    startPointIndex = RouteIndex(route, start->trackPointIndex);
    car->trackPointIndex = startPointIndex;
    car->x = start->x;
    car->y = 0;
    car->z = start->z;
    car->trackPointIndex =
        FindCarTrackSegment(AsRivalCar(car), route, car->trackPointIndex);
    if (car->trackPointIndex < 0) {
        car->trackPointIndex = startPointIndex;
        car->x = RoutePoint(route, startPointIndex)->x;
        car->z = RoutePoint(route, startPointIndex)->z;
    }

    car->bodyPitch = 0;
    car->bodyYaw = (ANGLE_THREE_QUARTER_TURN -
                    raceSeries * ANGLE_HALF_TURN -
                    RoutePoint(route, car->trackPointIndex)->angle) & ANGLE_MASK;
    car->bodyRoll = 0;
    car->bodyRollVelocity = 0;
    car->previousTrackPointIndex = car->trackPointIndex;
    car->headingAngle = car->bodyYaw;
    car->drive.targetHeading = car->headingAngle;

    SeedCarTrackProgress(AsRivalCar(car), route, setup->walkStart, 0, setup->reverse);
    StepCarTrackState(AsRivalCar(car), route, car->trackPointIndex, &trackLimits, setup->reverse, 1);
    car->previousTrackProgress = car->trackProgress;
    CopyPlayerBodyRotationToModel(car);
    car->modelY = car->y;

    CalculatePlayerBodyOffset(car);

    car->x = WrapSigned32((int64_t)car->x + car->motionX);
    car->z = WrapSigned32((int64_t)car->z + car->motionZ);
    car->facingBackwards = CarFacesBackwards(car, route);
}

void InitDriver(PlayerCarRuntime *car, GameCarSpec *spec,
                  CarPerformance *performance, const DriverStart *start) {
    ResetDriver(car, start);
    PlaceDriver(car, start);
    PrepareCarPerformance(&car->drive, spec, performance);
}
