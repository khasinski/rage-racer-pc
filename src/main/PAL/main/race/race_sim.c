#include "game/race_sim.h"
#include "game/race_lap.h"
#include "game/rival.h"
#include <string.h>

static int HasRoute(const TrackRoute *route) {
    return route != NULL && route->points != NULL &&
           route->count > 0 && route->length > 0;
}

int InitRaceSim(RaceSim *race, const TrackRoute *route,
                  const struct TrackEventData *events, s32 laps, int reverse) {
    if (race == NULL || !HasRoute(route) || laps < 1 ||
        laps > PLAYER_LAP_TIME_CAPACITY || (reverse != 0 && reverse != 1)) return 0;
    memset(race, 0, sizeof(*race));
    for (s32 seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) race->drivers[seat].variant = -1;
    race->route = *route;
    race->events = events;
    race->laps = laps;
    race->reverse = reverse;
    return 1;
}

int AddRaceDriver(RaceSim *race, s32 slot, const GameCarSpec *spec,
                    const DriverHull *hull, const CarHullPoint *roadCorners, const LaunchSpeedThreshold *threshold,
                    const TrackRivalStart *position, s32 walkStart, s16 manual,
                    s16 modelIndex, u32 seed) {
    if (race == NULL || race->phase != SIM_SETUP || !HasRoute(&race->route) || (u32)slot >= DRIVER_SEAT_LIMIT ||
        race->drivers[slot].status != SIM_EMPTY || spec == NULL || hull == NULL ||
        hull->points == NULL || hull->corners == NULL || roadCorners == NULL || threshold == NULL || position == NULL ||
        (manual != 0 && manual != 1) ||
        (u32)position->trackPointIndex >= (u32)race->route.count ||
        threshold->initial < 0 || threshold->sustain < 0) return 0;
    SimDriver *driver = &race->drivers[slot];
    driver->variant = -1;
    driver->spec = *spec;
    memcpy(driver->hull, hull->points, sizeof(driver->hull));
    memcpy(driver->corners, hull->corners, sizeof(driver->corners));
    memcpy(driver->roadCorners, roadCorners, sizeof(driver->roadCorners));
    driver->threshold = *threshold;
    const DriverStart start = {.route = &race->route, .position = position,
        .walkStart = walkStart, .reverse = race->reverse, .manual = manual,
        .modelIndex = modelIndex};
    InitDriver(&driver->car, &driver->spec, &driver->engine, &start);
    driver->random = seed;
    driver->status = SIM_DRIVING;
    return 1;
}

int AddRaceRival(RaceSim *race, s32 slot, s32 rivalSlot, const DriverHull *hull,
                 const CarHullPoint *rivalCorners,
                 const TrackRivalStart *position,
                 s32 walkStart, u16 model) {
    if (race == NULL || race->phase != SIM_SETUP || !HasRoute(&race->route) || race->events == NULL ||
        (u32)slot >= DRIVER_SEAT_LIMIT || (u32)rivalSlot >= RACE_CAR_SLOT_COUNT ||
        race->drivers[slot].status != SIM_EMPTY || hull == NULL || hull->points == NULL ||
        hull->corners == NULL || rivalCorners == NULL || position == NULL || position->activeFlag == -1 ||
        (u32)position->trackPointIndex >= (u32)race->route.count) return 0;
    for (s32 i = 0; i < DRIVER_SEAT_LIMIT; i++) {
        if (race->drivers[i].status != SIM_EMPTY && race->drivers[i].rival &&
            race->drivers[i].rivalSlot == rivalSlot) return 0;
    }
    SimDriver driver = {.variant = -1};
    if (!InitRival(AsRivalCar(&driver.car), &race->route, position, walkStart,
                   race->reverse, model)) return 0;
    ConfigureRival(AsRivalCar(&driver.car), race->events->rivalAiConfigs[race->reverse],
                   model, race->route.length, rivalSlot);
    SeedRivalSpeedKey(AsRivalCar(&driver.car), race->events->aiSpeedKeys[race->reverse]);
    memcpy(driver.hull, hull->points, sizeof(driver.hull));
    memcpy(driver.corners, hull->corners, sizeof(driver.corners));
    memcpy(driver.rivalCorners, rivalCorners, sizeof(driver.rivalCorners));
    driver.rival = 1;
    driver.rivalSlot = rivalSlot;
    driver.status = SIM_DRIVING;
    race->drivers[slot] = driver;
    return 1;
}

static int HasDrivers(const RaceSim *race) {
    for (s32 i = 0; i < DRIVER_SEAT_LIMIT; i++) {
        if (race->drivers[i].status == SIM_DRIVING &&
            race->drivers[i].car.activeFlag != -1) return 1;
    }
    return 0;
}

int StartRaceSim(RaceSim *race, u32 countdownTicks) {
    if (race == NULL || race->phase != SIM_SETUP || !HasDrivers(race)) return 0;
    race->countdown = countdownTicks;
    race->phase = countdownTicks ? SIM_COUNTDOWN : SIM_RACING;
    return 1;
}

static int IsFlag(int value) {
    return value == 0 || value == 1;
}

int SetRaceInput(RaceSim *race, s32 slot, const DriverInput *input) {
    if (race == NULL || input == NULL || (u32)slot >= DRIVER_SEAT_LIMIT ||
        race->phase == SIM_FINISHED || race->drivers[slot].status != SIM_DRIVING ||
        race->drivers[slot].rival || race->drivers[slot].car.activeFlag == -1 ||
        input->throttle < 0 || input->throttle > 256 || input->brake < 0 || input->brake > 256 ||
        input->steering.mode < STEERING_CENTER || input->steering.mode > STEERING_ANALOG ||
        input->steering.angle < -(13 * 512) || input->steering.angle > 13 * 512 ||
        !IsFlag(input->steering.left) || !IsFlag(input->steering.right) ||
        !IsFlag(input->shiftUp) || !IsFlag(input->shiftDown)) return 0;
    SimDriver *driver = &race->drivers[slot];
    const int up = driver->input.shiftUp || input->shiftUp;
    const int down = driver->input.shiftDown || input->shiftDown;
    driver->input = *input;
    driver->input.shiftUp = up;
    driver->input.shiftDown = down;
    return 1;
}

int RetireRaceDriver(RaceSim *race, s32 slot) {
    if (race == NULL || (u32)slot >= DRIVER_SEAT_LIMIT ||
        race->drivers[slot].status != SIM_DRIVING) return 0;
    race->drivers[slot].status = SIM_RETIRED;
    race->drivers[slot].car.activeFlag = -1;
    return 1;
}

static void StepCountdown(RaceSim *race) {
    for (s32 slot = 0; slot < DRIVER_SEAT_LIMIT; slot++) {
        SimDriver *driver = &race->drivers[slot];
        if (driver->status != SIM_DRIVING || driver->rival) continue;
        ApplyDriverInput(&driver->car, &driver->spec, &driver->input);
        const DriveContext context = {
            .point = &race->route.points[driver->car.trackPointIndex],
            .nextPoint = &race->route.points[(driver->car.trackPointIndex + 1) % race->route.count],
            .digitalSteering = driver->input.steering.mode == STEERING_DIGITAL,
        };
        StepCarDrivetrain(&driver->car, &driver->spec, &driver->engine, &context);
        driver->input.shiftUp = driver->input.shiftDown = 0;
    }
}

int StepRaceSim(RaceSim *race) {
    if (race == NULL || race->phase == SIM_SETUP || race->phase == SIM_FINISHED ||
        race->tick == UINT32_MAX || race->elapsed == UINT32_MAX) return 0;
    race->tick++;
    for (s32 i = 0; i < DRIVER_SEAT_LIMIT; i++) {
        if (race->drivers[i].status == SIM_DRIVING &&
            race->drivers[i].car.activeFlag == -1) RetireRaceDriver(race, i);
    }
    if (!HasDrivers(race)) { race->phase = SIM_FINISHED; return 1; }
    if (race->phase == SIM_COUNTDOWN) {
        if (race->tick % SIM_PHYSICS_INTERVAL == 0) StepCountdown(race);
        if (--race->countdown == 0) race->phase = SIM_RACING;
        return 1;
    }
    race->elapsed++;
    /* PAL retail physics is 25 Hz. The server clock is 50 Hz; applying the
     * unscaled retail step every tick would make the game run twice as fast. */
    if (race->elapsed % SIM_PHYSICS_INTERVAL != 0) return 1;
    DriverContext contexts[DRIVER_SEAT_LIMIT] = {0};
    DriverSeat seats[DRIVER_SEAT_LIMIT] = {0};
    for (s32 i = 0; i < DRIVER_SEAT_LIMIT; i++) {
        SimDriver *driver = &race->drivers[i];
        if (driver->status != SIM_DRIVING) continue;
        contexts[i] = (DriverContext){.spec = &driver->spec, .performance = &driver->engine,
            .route = &race->route, .events = race->events, .corners = driver->roadCorners,
            .launchThreshold = &driver->threshold, .reverse = race->reverse,
            .analogSteering = driver->input.steering.mode == STEERING_ANALOG,
            .drive = {.started = 1, .racing = 1,
                      .digitalSteering = driver->input.steering.mode == STEERING_DIGITAL}};
        seats[i] = (DriverSeat){.car = &driver->car, .context = &contexts[i],
            .input = driver->input, .rival = driver->rival, .rivalSlot = driver->rivalSlot, .rivalCorners = driver->rivalCorners, .hull = {driver->hull, driver->corners}, .random = driver->random,
            .wrongWayFrames = driver->wrongWayFrames};
    }
    if (!StepDriverField(seats, DRIVER_SEAT_LIMIT)) return 0;
    for (s32 i = 0; i < DRIVER_SEAT_LIMIT; i++) {
        SimDriver *driver = &race->drivers[i];
        if (driver->status != SIM_DRIVING) continue;
        driver->step = seats[i].step;
        driver->stepTick = race->tick;
        driver->crashed = seats[i].crashed;
        /* Track search can deactivate a car without producing a lap event.
         * Such a seat must not keep the room racing indefinitely. */
        if (driver->car.activeFlag == -1) {
            RetireRaceDriver(race, i);
            continue;
        }
        driver->random = seats[i].random;
        if (driver->car.facingBackwards != race->reverse) {
            if (driver->wrongWayFrames < INT32_MAX) driver->wrongWayFrames++;
        } else {
            driver->wrongWayFrames = 0;
        }
        driver->input.shiftUp = driver->input.shiftDown = 0;
        const LapEvent event = AdvanceCarLap(&driver->car, race->route.length, race->laps);
        if (event == LAP_COMPLETED || event == LAP_FINISHED) {
            driver->lapTicks[driver->car.lap - 2] = race->elapsed - driver->lapStarted;
        }
        if (event != LAP_NONE) driver->lapStarted = race->elapsed;
        if (event == LAP_FINISHED) {
            driver->status = SIM_DRIVER_FINISHED;
            driver->finishTick = race->elapsed;
            /* Same-tick finishes are ordered by stable seat index. */
            driver->place = ++race->finishCount;
            driver->car.activeFlag = -1;
        }
    }
    if (!HasDrivers(race)) race->phase = SIM_FINISHED;
    return 1;
}

s32 RacePosition(const RaceSim *race, s32 seat) {
    if (!race || (u32)seat >= DRIVER_SEAT_LIMIT) return 0;
    const SimDriver *subject = &race->drivers[seat];
    if (subject->status == SIM_DRIVER_FINISHED) return subject->place;
    if (subject->status != SIM_DRIVING || subject->car.activeFlag == -1) return 0;
    const int64_t progress = (int64_t)subject->car.progressA + subject->car.progressB;
    s32 place = 1;
    for (s32 other = 0; other < DRIVER_SEAT_LIMIT; ++other) {
        if (other == seat) continue;
        const SimDriver *driver = &race->drivers[other];
        if (driver->status == SIM_DRIVER_FINISHED) {
            ++place;
        } else if (driver->status == SIM_DRIVING && driver->car.activeFlag != -1) {
            const int64_t ahead = (int64_t)driver->car.progressA + driver->car.progressB;
            if (ahead > progress || (ahead == progress && other < seat)) ++place;
        }
    }
    return place;
}

static s32 TickMilliseconds(u32 ticks) {
    const uint64_t milliseconds = (uint64_t)ticks * 1000 / SIM_TICK_RATE;
    return milliseconds > INT32_MAX ? INT32_MAX : (s32)milliseconds;
}

s32 RaceTime(const RaceSim *race, s32 seat) {
    if (!race || (u32)seat >= DRIVER_SEAT_LIMIT) return -1;
    const SimDriver *driver = &race->drivers[seat];
    if (driver->status == SIM_DRIVER_FINISHED) return TickMilliseconds(driver->finishTick);
    if (driver->status != SIM_DRIVING || driver->car.activeFlag == -1) return -1;
    return TickMilliseconds(race->elapsed);
}

s32 RaceLapTime(const RaceSim *race, s32 seat, s32 lap) {
    if (!race || (u32)seat >= DRIVER_SEAT_LIMIT || (u32)lap >= PLAYER_LAP_TIME_CAPACITY ||
        lap >= race->laps) return -1;
    const SimDriver *driver = &race->drivers[seat];
    if (driver->status != SIM_DRIVER_FINISHED &&
        (driver->status != SIM_DRIVING || driver->car.activeFlag == -1)) return -1;
    if (driver->car.lap > lap + 1) return TickMilliseconds(driver->lapTicks[lap]);
    if (driver->car.lap == lap + 1 && driver->status == SIM_DRIVING &&
        race->phase == SIM_RACING && race->elapsed >= driver->lapStarted)
        return TickMilliseconds(race->elapsed - driver->lapStarted);
    return -1;
}
