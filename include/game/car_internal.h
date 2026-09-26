#ifndef GAME_CAR_INTERNAL_H
#define GAME_CAR_INTERNAL_H

#include "common.h"
#include "game/car.h"
#include "game/car_drive.h"
#include "game/car_shift.h"
#include "game/car_collision_internal.h"
#include "game/car_motion_internal.h"
#include "game/car_track_internal.h"
#include "game/integer.h"
#include "game/car_runtime_state.h"
#include "game/render.h"
#include "game/vector.h"

extern LaunchSpeedThreshold
    g_LaunchSpeedThresholds[CAR_LAUNCH_THRESHOLD_COUNT];


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
void SeedCarAiSpeedKeys(void);
void SlowRivalAhead(s32 rank);
void SteerCarAlongRoute(GameCarRuntime *car);
void SteerCarToTrackLine(PlayerCarRuntime *car);
void UpdateCarAiTargetSpeed(GameCarRuntime *car, s32 carIndex);
void UpdateCarTrafficAvoidance(GameCarRuntime *car, s32 carIndex);
void UpdateRivalRubberBand(void);
/* Whether the player's heading differs from the local road direction by more
 * than a quarter turn. */
void ReadPlayerCarInput(GameCarDrive *drive);
/* Pick the player's gear for this frame. Alternate controllers keep their two
 * shift buttons in the second half of the mapping table. */
void ShiftPlayerGears(PlayerCarRuntime *car, int useAlternateMapping);
void UpdateCarDrivetrain(PlayerCarRuntime *car);
void PlayCarDrivingVoice(const PlayerCarRuntime *car, const GameCarSpec *spec);
void PlayCarLaunchVoice(const PlayerCarRuntime *car);
void PlayCarAirborneVoice(const PlayerCarRuntime *car);
void PlayCarStandingStartVoice(const PlayerCarRuntime *car);
void UpdateCarTravelVelocity(GameCarRuntime *car);
void PlayPlayerLandingCue(s32 landingFrames, int audible);
void PlayPlayerContactCue(const PlayerCarRuntime *car, s32 skid, s32 slip, int audible);
void UpdatePlayerEnginePresentation(const PlayerCarRuntime *car, const GameCarSpec *spec, int finished);
void UpdatePlayerSteeringTarget(PlayerCarRuntime *car);

#endif
