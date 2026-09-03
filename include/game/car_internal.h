#ifndef GAME_CAR_INTERNAL_H
#define GAME_CAR_INTERNAL_H

#include "common.h"
#include "game/car.h"
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

extern u32 g_CarModelSlot;
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
};

/* Final per-frame visual/vertical motion pass over the rival car slots. */
void UpdateRivalBodyMotion(void);
/* Shared 12-bit wheel phase and high-speed blur flag update. */
void UpdateCarWheelRotation(GameCarRuntime *car);
/* Heading from a car position to a laterally offset interpolated track point. */
s32 CalculateTrackOffsetHeading(s32 pointIndex, s32 segmentFraction,
                                s32 carX, s32 carZ, s32 lateralOffset);
/* Internal course-event query used by the crest-hop state machine. */
s32 GetCarCrestTrigger(GameCarRuntime *car);
/* World translation and steering/body-lean pass over the rival car slots. */
void MoveRivalCars(void);
void AccelerateRaceRivals(void);
void AccelerateAttractRivals(void);
void PlaceRivalCarsOnTrack(void);

typedef struct {
    s32 longitudinalResistance;
    s32 motionResistance;
    s32 throttleAcceleration;
} CarDrivetrainLoads;

typedef struct CarCollisionHit {
    s32 region;
    s32 sampleIndex;
    s32 quadIndex;
} CarCollisionHit;

enum {
    CAR_COLLISION_QUAD_COUNT = 4,
    LAST_FRONT_COLLISION_REGION = 2,
};

CarCollisionHit FindFirstCarCollisionQuad(
    const CarCollisionPoint
        grid[CAR_COLLISION_QUAD_COUNT][CAR_COLLISION_QUAD_COUNT],
    const CarCollisionPoint *points, s32 count);

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
