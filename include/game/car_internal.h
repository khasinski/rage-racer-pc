#ifndef GAME_CAR_INTERNAL_H
#define GAME_CAR_INTERNAL_H

#include "common.h"
#include "game/car.h"
#include "game/render.h"
#include "game/vector.h"

typedef union RaceIntroCameraCoordinate {
    s32 word;
    struct {
        u16 value;
        u16 reserved;
    } half;
} RaceIntroCameraCoordinate;

typedef struct RaceIntroCameraKey {
    RaceIntroCameraCoordinate x;
    RaceIntroCameraCoordinate y;
    RaceIntroCameraCoordinate z;
    s32 mode;
    s16 startFrame;
    s16 duration;
} RaceIntroCameraKey;

struct RaceIntroCameraScript {
    s16 firstKeyIndex[2];
    RaceIntroCameraKey keys[1];
};

typedef struct LaunchSpeedThreshold {
    s16 initial;
    s16 sustain;
} LaunchSpeedThreshold;

enum {
    CAR_LAUNCH_THRESHOLD_COUNT = 5,
};

static inline s32 NormalizeCarLaunchThresholdIndex(s32 index) {
    index %= CAR_LAUNCH_THRESHOLD_COUNT;
    return index < 0 ? index + CAR_LAUNCH_THRESHOLD_COUNT : index;
}

extern u32 g_CarModelSlot;
extern RaceIntroCameraKey *g_RaceIntroCameraCursor;
extern LaunchSpeedThreshold
    g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT];
enum {
    CAR_TORQUE_BAND_COUNT = 10,
};
extern s16 g_TorqueBandEnd[CAR_TORQUE_BAND_COUNT];
extern s16 g_TorqueLossBandEnd[CAR_TORQUE_BAND_COUNT];

/* Final per-frame visual/vertical motion pass over the rival car slots. */
void UpdateRivalBodyMotion(void);
/* World translation and steering/body-lean pass over the rival car slots. */
void MoveRivalCars(void);
void AccelerateRaceRivals(void);
void AccelerateAttractRivals(void);
void PlaceRivalCarsOnTrack(void);

typedef struct {
    s32 accelerationResistance;
    s32 steeringResistance;
    s32 throttleAcceleration;
} CarDrivetrainLoads;

typedef struct CarCollisionHit {
    s32 region;
    s32 sampleIndex;
    s32 quadIndex;
} CarCollisionHit;

CarCollisionHit FindFirstCarCollisionQuad(
    const CarCollisionPoint grid[4][4], const CarCollisionPoint *points,
    s32 count);

void UpdateCarSteeringGrip(PlayerCarRuntime *car, const GameCarSpec *spec,
                           s32 gripBudget);
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

s32 InterpolateCarTrackValue(s32 start, s32 end, s32 alongSegment,
                             s16 segmentLength);
s32 CarTrackFixed12ToInteger(s32 value);
s32 ProjectCarTrackAxis(s32 value);
s16 InterpolateCarTrackHeading(s16 pointHeading, s16 nextHeading,
                               s32 swept, s16 arcSpan);

#endif
