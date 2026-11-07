#ifndef GAME_DRIVER_H
#define GAME_DRIVER_H

#include "game/car_control.h"
#include "game/car_drive.h"
#include "game/car_collision_internal.h"

struct TrackEventData;

/* Borrowed immutable data and explicit conditions for this driver. */
typedef struct DriverContext {
    const GameCarSpec *spec;
    const CarPerformance *performance;
    const TrackRoute *route;
    const LaunchSpeedThreshold *launchThreshold;
    const CarHullPoint *corners; /* contact with the road, not other cars */
    const struct TrackEventData *events;
    DriveContext drive;
    int reverse;
    int analogSteering;
} DriverContext;

typedef struct DriverStart {
    const TrackRoute *route;
    const TrackRivalStart *position;
    s32 walkStart;
    int reverse;
    s16 manual;
    s32 launchThresholdIndex;
    s16 modelIndex;
} DriverStart;

/* Resets all transient state and prepares this driver's private engine data.
 * Missing grid/route data leaves the reset car unplaced, as in the client. */
void InitDriver(PlayerCarRuntime *car, GameCarSpec *spec,
                  CarPerformance *performance, const DriverStart *start);

typedef struct DriverStep {
    s32 skid;
    int motionFinished;
    /* Zero without a landing; otherwise its jump duration, captured before
     * subsequent pose/crest updates. Presentation can consume it afterwards. */
    s32 landingFrames;
    s32 skidAngle;
} DriverStep;

/* Required: valid specification/performance, route with points/count/length,
 * launch threshold, four hull corners and caller-owned RNG. No audio or input
 * device calls. Move all drivers before the race owner resolves car pairs. */
DriverStep MoveDriver(PlayerCarRuntime *car, const DriverInput *input,
                       const DriverContext *context, u32 *random);
/* After drivetrain/motion: integrate position, update track progress/contact
 * and shift pose. Shared with a client that presents sound between stages. */
DriverStep AdvanceDriver(PlayerCarRuntime *car, const DriverContext *context, u32 *random);
/* Complete the step after car-pair response. crash is supplied by the race
 * owner; this function never chooses or updates another driver. */
void FinishDriver(PlayerCarRuntime *car, const DriverContext *context,
                   u32 *random, s32 crash, DriverStep *step);

/* Slot order is stable for the lifetime of a race. Empty/inactive seats are
 * skipped. AI seats use authored rival physics and the same route/events. */
typedef struct DriverSeat {
    PlayerCarRuntime *car;
    const DriverContext *context;
    DriverInput input;
    int rival;
    s32 rivalSlot; /* authored AI slot, independent of the field seat */
    const CarHullPoint *rivalCorners; /* AI/AI uses a different hull scale */
    DriverHull hull; /* car-to-car collision geometry */
    u32 random;
    s32 wrongWayFrames;
    DriverStep step;
    int crashed;
} DriverSeat;

enum { DRIVER_SEAT_LIMIT = RACE_CAR_SLOT_COUNT + 1 };
/* All active seats must share the route and direction. Returns zero for an
 * invalid field before modifying any seat/car. Contacts are detected before
 * any pair response, then resolved in stable slot order. */
int StepDriverField(DriverSeat *seats, s32 count);

#endif
