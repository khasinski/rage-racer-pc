#ifndef GAME_CAR_DRIVE_H
#define GAME_CAR_DRIVE_H

#include "game/car.h"
#include "game/track.h"

static inline s32 NormalizeCarLaunchThresholdIndex(s32 index) {
    index %= CAR_LAUNCH_THRESHOLD_COUNT;
    return index < 0 ? index + CAR_LAUNCH_THRESHOLD_COUNT : index;
}

/* Immutable road samples for this car's segment. Null samples mean there is
 * no road data. The race owner resolves indices before stepping the car. */
typedef struct DriveContext {
    const GameTrackPoint *point;
    const GameTrackPoint *nextPoint;
    int racing;
    int digitalSteering;
    int started;
} DriveContext;

enum { CAR_STOPPED_SPEED_THRESHOLD = 8 };

typedef struct CarDrivetrainLoads {
    s32 longitudinalResistance;
    s32 motionResistance;
    s32 throttleAcceleration;
} CarDrivetrainLoads;

/* Normalizes the supplied spec and prepares reusable engine data. No global
 * tables are modified; prepare once before stepping drivers with this spec. */
void PrepareCarPerformance(GameCarDrive *drive, GameCarSpec *spec,
                           CarPerformance *performance);
void ReadCarEngineTorque(const GameCarDrive *drive, const GameCarSpec *spec,
                         const CarPerformance *performance,
                         const s32 *gearCurve, s32 *netTorque, s32 *bandScale);
s32 CalculateCarInitialAcceleration(const GameCarDrive *drive, s32 gearRatio);
/* Engine and longitudinal dynamics only. The race owner runs the selected
 * motion handler afterwards; no rendering, sound or device calls occur here. */
void StepCarDrivetrain(PlayerCarRuntime *car, const GameCarSpec *spec,
                       const CarPerformance *performance,
                       const DriveContext *context);
/* Engine followed by the selected motion handler. The caller owns the RNG,
 * immutable track and launch threshold. Does not integrate world position or
 * resolve contacts; nonzero reports completion of airborne/wheelspin motion.
 * random must be valid when standing-start wheelspin consumes samples. */
int StepCarDynamics(PlayerCarRuntime *car, const GameCarSpec *spec,
                     const CarPerformance *performance,
                     const DriveContext *context, const TrackRoute *route,
                     const LaunchSpeedThreshold *threshold, u32 *random);
/* Motion only, after the engine step. The client can present the engine's
 * intermediate state before invoking this same dispatcher as the server. */
int StepCarMotion(PlayerCarRuntime *car, const GameCarSpec *spec,
                  const TrackRoute *route, const LaunchSpeedThreshold *threshold,
                  u32 *random);
/* Gear-shift body pitch; consumes RNG only for an active shift below the limit. */
void StepCarShiftPitch(PlayerCarRuntime *car, const GameCarSpec *spec, u32 *random);
void UpdateCarTravelVelocity(GameCarRuntime *car);
void StepCarDriving(PlayerCarRuntime *car, const LaunchSpeedThreshold *threshold);
enum { CAR_STANDING_START_MIN_SPIN = 11 };
/* Random samples are supplied by the race owner, so rooms need not share a
 * generator. The legacy adapter samples only while wheelspin is active. */
/* Returns nonzero when wheelspin finishes on this step. */
int StepCarStandingStart(PlayerCarRuntime *car, s32 verticalRandom,
                          s32 lateralRandom);
int StepCarAirborne(PlayerCarRuntime *car);
void StepCarLaunch(PlayerCarRuntime *car, const GameCarSpec *spec,
                   const TrackRoute *route);

void UpdateCarSteeringGrip(PlayerCarRuntime *car, const GameCarSpec *spec,
                           const DriveContext *context, s32 gripBudget);
CarDrivetrainLoads CalculateCarDrivetrainLoads(
    PlayerCarRuntime *car, const GameCarSpec *spec, const DriveContext *context,
    s32 netTorque, s32 bandScale, s32 initialAcceleration);

#endif
