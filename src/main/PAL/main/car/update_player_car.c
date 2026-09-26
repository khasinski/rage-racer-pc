#include "game/car_control.h"
#include "game/car_drive.h"
#include "game/race.h"
#include "game/car.h"
#include "game/car_internal.h"
#include "game/integer.h"
#include "game/random.h"
#include "game/driver.h"
#include "game/track_internal.h"
#include "game/state.h"


/* Per-frame player physics orchestration and track contact. */
void UpdatePlayerCar(PlayerCarRuntime *car) {
    const TrackRoute route = {.points = g_TrackPoints, .arcs = g_TrackArcCenters,
        .count = g_TrackPointCount, .length = g_TrackLength};
    const DriverContext context = {.spec = g_CarSpec, .route = &route,
        .events = g_TrackEventData, .corners = g_CarCornerOffsets,
        .reverse = g_RaceSeries != 0, .analogSteering = g_PadType == PAD_TYPE_NEGCON,
        .drive.started = g_RacePhase >= RACE_PHASE_ACTIVE};
    const DriverInput input = ReadDriverInput();
    car->facingBackwards = CarFacesBackwards(car, &route);
    ApplyDriverInput(car, g_CarSpec, &input);
    UpdateCarDrivetrain(car);

    DriverStep step = AdvanceDriver(car, &context, &g_RandomSeed);
    const s32 crash = CollidePlayerWithCars(car);
    FinishDriver(car, &context, &g_RandomSeed, crash, &step);
    const int audible = g_RacePhase <= RACE_PHASE_ACTIVE;
    PlayPlayerLandingCue(step.landingFrames, audible);
    PlayPlayerContactCue(car, step.skid, step.skidAngle, audible);

    UpdatePlayerEnginePresentation(car, g_CarSpec, g_RacePhase >= RACE_PHASE_FINISHED);
}
