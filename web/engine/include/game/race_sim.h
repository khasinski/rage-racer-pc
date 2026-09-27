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
    u32 inputTick; /* Last tick that consumed human controls, including countdown. */
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

/* Local rollback state; immutable specs/hulls/track stay owned by RaceSim.
 * Identity pointers are borrowed guards, never a network serialization. */
typedef struct DriverFrame {
    PlayerCarRuntime car;
    DriverInput input;
    DriverStep step;
    u32 stepTick, inputTick, random, lapStarted, lapTicks[PLAYER_LAP_TIME_CAPACITY], finishTick;
    s32 crashed, rival, variant, rivalSlot, place, wrongWayFrames;
    SimDriverStatus status;
} DriverFrame;
typedef struct RaceFrame {
    const void *track, *events;
    s32 laps, reverse, finishCount;
    u32 tick, countdown, elapsed;
    SimRacePhase phase;
    DriverFrame drivers[DRIVER_SEAT_LIMIT];
} RaceFrame;
/* Checks the whole mutable frame against its owner without modifying either. */
int ValidRaceFrame(const RaceSim *race, const RaceFrame *frame);
int SaveRaceFrame(const RaceSim *race, RaceFrame *frame);
/* Same owned track/configuration only. Invalid frames leave race unchanged. */
int RestoreRaceFrame(RaceSim *race, const RaceFrame *frame);
enum { RACE_FRAME_WIRE_VERSION = 1, RACE_FRAME_WIRE_SIZE = 1 + 7 * 4 + DRIVER_SEAT_LIMIT * (116 + 412) };
/* Exact little-endian checkpoint schema. No pointers, enum layout or implicit
 * padding on wire. Decode binds identity to the receiver's already-owned setup.
 * Invalid input leaves output untouched. These bytes are not yet a TCP packet. */
int EncodeRaceFrame(const RaceSim *race, uint8_t *wire, size_t size);
int DecodeRaceFrame(const RaceSim *race, const uint8_t *wire, size_t size, RaceFrame *out);


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
/* Replace a human driver's specification before start and rebuild all derived
 * engine/drive values. Does not move the car or change its model/input/seed.
 * AI, empty seats and running races are rejected without mutation. */
int ConfigureRaceDriver(RaceSim *race, s32 slot, const GameCarSpec *spec);
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
