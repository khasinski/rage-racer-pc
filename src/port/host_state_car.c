/*
 * Retail state belonging to the cars: the player's and the rivals' physical
 * condition, the drivetrain, the tyres and their grip, the collision and
 * knockback bookkeeping, and the rival AI's view of the race.
 *
 * The camera's subject is here too, since the car code sets it. Order is
 * retail's address order.
 */

#include <stddef.h>

#include "common.h"
#include "game/car.h"
#include "game/car_runtime_state.h"
#include "game/track.h"

s32 g_ShiftTargetSpeed;
s32 g_RoadGrade;
u8 g_RoadGradeReserved[12] __attribute__((aligned(16)));
CarHullPoint g_PlayerHullPoints[6] __attribute__((aligned(16))) = {
    {-32, 64}, {32, 64}, {-24, -72}, {24, -72}, {-32, 16}, {32, 16}
};
CarHullPoint g_OpponentHullCorners[4] __attribute__((aligned(16))) = {
    {-26, 96}, {26, 96}, {-26, -16}, {26, -16}
};
CarHullPoint g_CarCornerOffsets[4] __attribute__((aligned(16))) = {
    {-15, 20}, {15, 20}, {-8, -10}, {8, -10}
};
LaunchSpeedThreshold g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT]
    __attribute__((aligned(16))) = {
        {960, 320}, {960, 320}, {960, 320}, {960, 320}, {960, 320}
    };
s16 g_LaunchEnergyThresholds[6] __attribute__((aligned(16))) = {
    450, 900, 1000, 1300, 1550, 0
};
CarCollisionPoint g_CarCollisionCorners[4] __attribute__((aligned(16))) = {
    {-96, 512}, {96, 512}, {-96, -128}, {96, -128}
};
/* The retail data dump grouped these bytes under g_CarCollisionCorners even
 * though collision code only addresses the four points above. Keep them
 * losslessly until their original owner is identified. */
u8 g_CarCollisionTrailingData[60] __attribute__((aligned(16))) = {
    0x77,0xba,0x00,0x00,0x4c,0x17,0x00,0x00,0x7a,0x31,0x00,0x00,
    0x00,0x00,0x00,0x00,0x50,0x00,0x4b,0x00,0x38,0xbc,0x00,0x00,
    0x5b,0x17,0x00,0x00,0xd7,0x30,0x00,0x00,0x01,0x00,0x00,0x00,
    0x5a,0x00,0x0f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
    0x00,0x00,0x00,0x00,0x01,0x00,0x00,0x00,0xff,0xff,0x00,0x00
};
s32 g_RaceIntroCameraTimer;
SVec g_RaceIntroCameraDelta __attribute__((aligned(16)));
u8 g_RaceIntroCameraReserved[8] __attribute__((aligned(16)));
GameTrackPoint *g_TrackPoints;
s16 g_RivalCueEnabled;
s32 g_TrackPointCount;
s16 g_PeakOutputRpm;
s32 g_DriveBoostTimer;
s16 g_PlayerAutoSteer;
s32 g_StandingStartSpin;
s16 g_TrackZoneDark;
s32 g_EngineRpm;
RaceIntroCameraScript *g_RaceIntroCameraScript;
GameCarRuntime g_CameraCar __attribute__((aligned(16)));
s32 g_CameraCarSeedYaw;
RaceIntroCameraKey *g_RaceIntroCameraCursor;
s32 g_RaceSeries;
s32 g_TachoNeedleFlash;
GameCarRuntime *g_RankedCars[4] __attribute__((aligned(16)));
s32 g_TrackLength;
s16 g_TorqueBandEnd[CAR_TORQUE_BAND_COUNT] __attribute__((aligned(16)));
u16 g_HudGlyphClut;
TrackEventData *g_TrackEventData;
s16 g_TorqueLossBandEnd[CAR_TORQUE_BAND_COUNT]
    __attribute__((aligned(16)));
s32 g_EngineRpmJitter;
s32 g_EngineRpmSnapshot;
GameCarSpec *g_CarSpec;
s16 g_PeakOutputValue;
s16 g_GripLossTimer;
s32 g_RivalCueFlags;
s32 g_ShiftTargetRpm;
s16 g_DragScale;
s16 g_RedlineToPeakRpmHalf;
s16 g_PeakToRevLimitRpmHalf;
s16 g_RivalCueCooldown3;
s32 g_ClosestRivalRank;
GearCurveRow g_GearTorqueCurve[7] __attribute__((aligned(16)));
s16 g_StandingStartState;
s32 g_ShiftSoundLevel;
s16 g_SteerHoldFrames;
s32 g_AutoShiftCooldown;
