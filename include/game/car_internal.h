#ifndef GAME_CAR_INTERNAL_H
#define GAME_CAR_INTERNAL_H

#include "common.h"
#include "game/car.h"
#include "game/car_collision_internal.h"
#include "game/car_motion_internal.h"
#include "game/integer.h"
#include "game/car_runtime_state.h"
#include "game/render.h"
#include "game/vector.h"

static inline s32 NormalizeCarLaunchThresholdIndex(s32 index) {
    index %= CAR_LAUNCH_THRESHOLD_COUNT;
    return index < 0 ? index + CAR_LAUNCH_THRESHOLD_COUNT : index;
}

static inline s32 CalculateAirborneEngineRpm(const GameCarSpec *spec,
                                             s32 gear, s32 speed) {
    s32 gearRatio = GetPositiveCarGearRatio(spec, gear);
    s32 wheelRpm = WrapSigned32((int64_t)speed * 160) / 1168;

    return WrapSigned32((int64_t)wheelRpm * 10000) / gearRatio;
}

static inline s16 CalculateCarRpmDelta(s32 targetRpm, s32 currentRpm) {
    return WrapSigned16((u16)targetRpm - (u16)currentRpm);
}

static inline s16 ClampCarGear(s32 gear, s32 topGear) {
    if (topGear < CAR_FIRST_FORWARD_GEAR) {
        topGear = CAR_FIRST_FORWARD_GEAR;
    } else if (topGear > CAR_FORWARD_GEAR_COUNT) {
        topGear = CAR_FORWARD_GEAR_COUNT;
    }
    if (gear < CAR_FIRST_FORWARD_GEAR) {
        return CAR_FIRST_FORWARD_GEAR;
    }
    return (s16)(gear > topGear ? topGear : gear);
}

extern RaceIntroCameraKey *g_RaceIntroCameraCursor;
extern LaunchSpeedThreshold
    g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT];
extern s16 g_TorqueBandEnd[CAR_TORQUE_BAND_COUNT];
extern s16 g_TorqueLossBandEnd[CAR_TORQUE_BAND_COUNT];

enum {
    CAR_WHEEL_GROUND_OFFSET = 8,
    CAR_JUMP_RISE_CURVE = 72,
    CAR_JUMP_FALL_CURVE = 216,
    CAR_JUMP_CURVE_SCALE = 100,
    CAR_AIRBORNE_SHIFT_FRAMES = 20,
};

/* Final per-frame visual/vertical motion pass over the rival car slots. */
void UpdateRivalBodyMotion(void);
/* Shared 12-bit wheel phase and high-speed blur flag update. */
void UpdateCarWheelRotation(GameCarRuntime *car);
/* Heading from a car position to a laterally offset interpolated track point. */
s32 CalculateTrackOffsetHeading(s32 pointIndex, s32 segmentFraction,
                                s32 carX, s32 carZ, s32 lateralOffset);
/* World translation and steering/body-lean pass over the rival car slots. */
void MoveRivalCars(void);
void AccelerateRaceRivals(void);
void AccelerateAttractRivals(void);
void InitRivalCar(GameCarRuntime *car, s32 gridPosition,
                  const RaceGridSlot *grid);
void InitRivalCarAi(GameCarRuntime *car, s32 gridPosition,
                    const RaceGridSlot *grid);
void PlaceRivalCarsOnTrack(void);
void ApplyCarRacingLineHint(GameCarRuntime *car, s32 carIndex);
void ClampCarLateralOffset(GameCarRuntime *car, s32 rivalSlot);
void RankContenders(void);
void SeedCarRouteMarkers(void);
void SlowRivalAhead(s32 rank);
void SteerCarAlongRoute(GameCarRuntime *car);
void SteerCarToTrackLine(PlayerCarRuntime *car);
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 carIndex);
void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex);
void UpdateRivalRubberBand(void);
/* Whether the player's heading differs from the local road direction by more
 * than a quarter turn. */
s32 IsCarFacingBackwards(const PlayerCarRuntime *car);
typedef struct {
    s32 longitudinalResistance;
    s32 motionResistance;
    s32 throttleAcceleration;
} CarDrivetrainLoads;

void UpdateCarSteeringGrip(PlayerCarRuntime *car, const GameCarSpec *spec,
                           s32 gripBudget);
void AdvanceCarJumpArc(GameCarRuntime *car, s32 groundHeight);
CarDrivetrainLoads CalculateCarDrivetrainLoads(
    PlayerCarRuntime *car, const GameCarSpec *spec, s32 netTorque,
    s32 bandScale, s32 initialAcceleration);
void ReadCarEngineTorque(const GameCarDrive *drive, const GameCarSpec *spec,
                         const s32 *gearCurve, s32 *netTorque,
                         s32 *bandScale);
s32 CalculateCarInitialAcceleration(const GameCarDrive *drive,
                                    s32 gearRatio);
void UpdateCarGearShiftState(PlayerCarRuntime *car, const GameCarSpec *spec,
                             s32 *acceleration);
void ReadPlayerCarInput(GameCarDrive *drive);
/* Pick the player's gear for this frame. Alternate controllers keep their two
 * shift buttons in the second half of the mapping table. */
void ShiftPlayerGears(PlayerCarRuntime *car, int useAlternateMapping);
void UpdateCarDrivetrain(PlayerCarRuntime *car);
void UpdateCarDriving(PlayerCarRuntime *car);
void UpdateCarLaunch(PlayerCarRuntime *car);
void UpdateCarAirborne(PlayerCarRuntime *car);
void UpdateCarStandingStart(PlayerCarRuntime *car);
void UpdateCarTravelVelocity(GameCarRuntime *car);
void UpdatePlayerJump(PlayerCarRuntime *car, s32 groundHeight);
void UpdatePlayerEnginePresentation(PlayerCarRuntime *car);
void MeasurePlayerTrackLimits(const Matrix *toTrack,
                              CarTrackLimits *limits);
void ApplyPlayerContactResponse(PlayerCarRuntime *car, s32 skid, s32 crash);
void UpdatePlayerSteeringTarget(PlayerCarRuntime *car);
void UpdatePlayerControlFeedback(PlayerCarRuntime *car);
void CalculatePlayerBodyOffset(PlayerCarRuntime *car);
s32 ResolvePlayerTrackContact(PlayerCarRuntime *car);
void PrepareAirborneDrivetrain(PlayerCarRuntime *car);

s32 InterpolateCarTrackValue(s32 start, s32 end, s32 alongSegment,
                             s16 segmentLength);
s32 CarTrackFixed12ToInteger(s32 value);
s32 ProjectCarTrackAxis(s32 value);
s16 InterpolateCarTrackHeading(s16 pointHeading, s16 nextHeading,
                               s32 swept, s16 arcSpan);

#endif
