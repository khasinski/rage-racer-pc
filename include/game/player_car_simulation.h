#ifndef GAME_PLAYER_CAR_SIMULATION_H
#define GAME_PLAYER_CAR_SIMULATION_H

#include "common.h"
#include "game/car_types.h"
#include "game/track.h"

/* Per-frame environment consumed by the player-car simulation. Mutable
 * legacy scalars are explicit pointers until their ownership moves into a
 * longer-lived race context. */
typedef struct PlayerCarSimulationContext {
    const GameCarSpec *carSpec;
    const GearCurveRow *gearTorqueCurves;
    const s16 *torqueBandEnd;
    const s16 *torqueLossBandEnd;
    const GameTrackPoint *trackPoints;
    const GameTrackArcCenter *trackArcCenters;
    const s16 *negconSteerRange;
    s32 trackPointCount;
    s32 racePhase;
    s32 playerAutoSteer;
    s32 negconMaxTwist;
    s32 standingStartSpin;
    s32 *roadGrade;
    s32 *shiftTargetRpm;
    s32 *shiftTargetSpeed;
    s16 *gripLossTimer;
    s32 *driveBoostTimer;
    s32 *autoShiftCooldown;
    s16 *steerHoldFrames;
    s16 *dragScale;
} PlayerCarSimulationContext;

#endif
