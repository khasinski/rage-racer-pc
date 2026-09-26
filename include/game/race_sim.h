#ifndef GAME_RACE_SIM_H
#define GAME_RACE_SIM_H

#include "game/driver.h"

enum { SIM_TICK_RATE = 50, SIM_PHYSICS_INTERVAL = 2 };
typedef enum SimRacePhase { SIM_SETUP, SIM_COUNTDOWN, SIM_RACING, SIM_FINISHED } SimRacePhase;
typedef enum SimDriverStatus { SIM_EMPTY, SIM_DRIVING, SIM_DRIVER_FINISHED, SIM_RETIRED } SimDriverStatus;

typedef struct SimDriver {
    PlayerCarRuntime car;
    GameCarSpec spec;
    CarPerformance engine;
    CarHullPoint hull[PLAYER_HULL_SAMPLE_COUNT];
    CarHullPoint corners[CAR_HULL_CORNER_COUNT];
    CarHullPoint roadCorners[CAR_HULL_CORNER_COUNT];
    CarHullPoint rivalCorners[CAR_HULL_CORNER_COUNT];
    LaunchSpeedThreshold threshold;
    DriverInput input;
    /* Last field-physics result. stepTick == 0 until the first field step.
     * Presentation tracks stepTick to consume events once; reading is inert.
     * Countdown/intermediate clock ticks retain the previous result. */
    DriverStep step;
    u32 stepTick;
    int crashed;
    int rival;
    s32 variant; /* Retail human variant, -1 when setup used a standalone spec. */
    s32 rivalSlot;
    u32 random;
    SimDriverStatus status;
    u32 lapStarted;
    u32 lapTicks[PLAYER_LAP_TIME_CAPACITY];
    u32 finishTick;
    s32 place;
    s32 wrongWayFrames;
} SimDriver;

/* No pointers into this object: copying it restores cars, inputs and RNG.
 * Track/event arrays are immutable borrowed data and must outlive the race. */
typedef struct RaceSim {
    TrackRoute route;
    const struct TrackEventData *events;
    SimDriver drivers[DRIVER_SEAT_LIMIT];
    SimRacePhase phase;
    s32 laps;
    int reverse;
    u32 tick;
    u32 countdown;
    u32 elapsed;
    s32 finishCount;
} RaceSim;

int InitRaceSim(RaceSim *race, const TrackRoute *route,
                  const struct TrackEventData *events, s32 laps, int reverse);
/* hull describes car-to-car contact; roadCorners describes track contact.
 * Both are copied, and invalid setup leaves the race unchanged. */
int AddRaceDriver(RaceSim *race, s32 slot, const GameCarSpec *spec,
                    const DriverHull *hull, const CarHullPoint *roadCorners, const LaunchSpeedThreshold *threshold,
                    const TrackRivalStart *position, s32 walkStart, s16 manual,
                    s16 modelIndex, u32 seed);
/* AI uses an authored behavior slot and the configuration for its logical model.
 * rivalCorners is the AI/AI hull, separate from hull's human/AI silhouette. Inactive
 * grid entries and duplicate AI slots are rejected before modifying the race. */
int AddRaceRival(RaceSim *race, s32 slot, s32 rivalSlot, const DriverHull *hull,
                 const CarHullPoint *rivalCorners,
                 const TrackRivalStart *position,
                 s32 walkStart, u16 model);
/* Start once from setup with at least one active seat. Zero skips countdown. */
int StartRaceSim(RaceSim *race, u32 countdownTicks);
/* Latest levels replace previous levels; gear edges accumulate until physics
 * consumes them, including inputs received between the two 50 Hz ticks.
 * Flags must be 0/1; pedals 0..256; steering center/digital/analog, with angle
 * within +/-13*512. Invalid commands leave the complete race unchanged. */
int SetRaceInput(RaceSim *race, s32 slot, const DriverInput *input);
/* Returns 1 for an advanced tick, 0 in setup/finished or at clock overflow.
 * Retiring the last driver closes the race on the next tick, without results. */
int StepRaceSim(RaceSim *race);
/* Read-only HUD position, 1-based; zero for missing/retired/inactive seats.
 * Finished seats retain their recorded place; progress ties use seat order. */
s32 RacePosition(const RaceSim *race, s32 seat);
/* HUD milliseconds from the authoritative 50 Hz clock. Lap indices are
 * zero-based; absent/retired/unstarted laps return -1. Large times saturate. */
s32 RaceTime(const RaceSim *race, s32 seat);
s32 RaceLapTime(const RaceSim *race, s32 seat, s32 lap);
/* Retires an active seat only; preserves other seats and recorded results. */
int RetireRaceDriver(RaceSim *race, s32 slot);

#endif
