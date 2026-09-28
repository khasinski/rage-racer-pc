/* Race server module: the authoritative simulation for every room of the
 * multiplayer server (web/server). One imported archive is shared by any
 * number of independent races; each owns its track copy and RaceSim, is
 * stepped at 50 Hz by the server, and publishes its complete state as the
 * exact RaceFrame wire checkpoint that clients restore (race_frame.c). */
#include <emscripten/emscripten.h>

#include <stdlib.h>
#include <string.h>

#include "game/race_data.h"
#include "game/race_grid.h"
#include "game/race_sim.h"
#include "game/track_data.h"
#include "web_rules.h"

enum { SERVER_RACE_LIMIT = 64 };

typedef struct ServerRace {
    TrackData *track; /* borrowed by sim for the race's lifetime */
    RaceSim sim;
    int humans;
} ServerRace;

static RaceData *s_archive;
static ServerRace *s_races[SERVER_RACE_LIMIT + 1]; /* handle 0 is never used */
static uint8_t s_frame[RACE_FRAME_WIRE_SIZE];

const RaceData *WebLoadedArchive(void) { return s_archive; }

EMSCRIPTEN_KEEPALIVE int rs_load_disc(const char *path) {
    RaceData *archive = LoadRaceDisc(path);
    if (!archive) return 0;
    for (int handle = 1; handle <= SERVER_RACE_LIMIT; ++handle)
        if (s_races[handle]) return 0; /* races borrow the current archive */
    FreeRaceData(s_archive);
    s_archive = archive;
    return 1;
}

static ServerRace *Race(int handle) {
    return handle > 0 && handle <= SERVER_RACE_LIMIT ? s_races[handle] : NULL;
}

/* humanSeats holds (variant, manual) per human in seat order. Returns a
 * handle, or 0 when the room's settings or cars are not a valid field. */
EMSCRIPTEN_KEEPALIVE int rs_create_race(int classIndex, int course, int reverse, int laps,
                                        int rivals, int humanCount, const int32_t *humanSeats) {
    WebSeat humans[DRIVER_SEAT_LIMIT];
    RaceEntrant entrants[DRIVER_SEAT_LIMIT];
    int handle = 1;
    if (!s_archive || !WebReadSeats(humanSeats, humanCount, humans) ||
        laps < 1 || laps > WEB_MAX_LAPS) return 0;
    while (handle <= SERVER_RACE_LIMIT && s_races[handle]) ++handle;
    if (handle > SERVER_RACE_LIMIT) return 0;
    if (!WebBuildField(s_archive, classIndex, course, reverse, humans, humanCount, rivals,
                       entrants)) return 0;
    ServerRace *race = calloc(1, sizeof(*race));
    if (!race) return 0;
    race->track = CopyRaceTrack(s_archive, classIndex, course);
    race->humans = humanCount;
    if (!race->track || !InitRaceGrid(&race->sim, s_archive, race->track, entrants, NULL,
                                      laps, reverse ? 1 : 0)) {
        FreeTrackData(race->track);
        free(race);
        return 0;
    }
    s_races[handle] = race;
    return handle;
}

/* Starts the countdown once every player has loaded the race. */
EMSCRIPTEN_KEEPALIVE int rs_start(int handle) {
    ServerRace *race = Race(handle);
    return race && StartRaceSim(&race->sim, WEB_COUNTDOWN_TICKS);
}

EMSCRIPTEN_KEEPALIVE void rs_free(int handle) {
    ServerRace *race = Race(handle);
    if (!race) return;
    FreeTrackData(race->track);
    free(race);
    s_races[handle] = NULL;
}

/* Latest controls for a human seat (web_rules.h wire words). */
EMSCRIPTEN_KEEPALIVE int rs_set_input(int handle, int seat, const int32_t *words) {
    ServerRace *race = Race(handle);
    DriverInput input;
    return race && seat >= 0 && seat < race->humans && WebDecodeInput(words, &input) &&
           SetRaceInput(&race->sim, seat, &input);
}

/* One 50 Hz tick. Returns the phase, or -1 for an unknown race. */
EMSCRIPTEN_KEEPALIVE int rs_tick(int handle) {
    ServerRace *race = Race(handle);
    if (!race) return -1;
    StepRaceSim(&race->sim);
    return (int)race->sim.phase;
}

EMSCRIPTEN_KEEPALIVE uint32_t rs_sim_tick(int handle) {
    ServerRace *race = Race(handle);
    return race ? race->sim.tick : 0;
}

/* Complete race state as RaceFrame wire bytes (rs_frame_size long). */
EMSCRIPTEN_KEEPALIVE uint8_t *rs_frame(int handle) {
    ServerRace *race = Race(handle);
    return race && EncodeRaceFrame(&race->sim, s_frame, sizeof(s_frame)) ? s_frame : NULL;
}
EMSCRIPTEN_KEEPALIVE int rs_frame_size(void) { return RACE_FRAME_WIRE_SIZE; }

/* A player who left keeps their place in the field but stops racing. */
EMSCRIPTEN_KEEPALIVE int rs_retire(int handle, int seat) {
    ServerRace *race = Race(handle);
    return race && seat >= 0 && seat < race->humans && RetireRaceDriver(&race->sim, seat);
}

/* Per seat: status (SimDriverStatus), place, race ms, best completed lap ms (-1 none). */
EMSCRIPTEN_KEEPALIVE int rs_seat_status(int handle, int seat) {
    ServerRace *race = Race(handle);
    return race && seat >= 0 && seat < DRIVER_SEAT_LIMIT ? (int)race->sim.drivers[seat].status : -1;
}
EMSCRIPTEN_KEEPALIVE int rs_seat_place(int handle, int seat) {
    ServerRace *race = Race(handle);
    return race ? RacePosition(&race->sim, seat) : 0;
}
EMSCRIPTEN_KEEPALIVE int rs_seat_time(int handle, int seat) {
    ServerRace *race = Race(handle);
    return race ? RaceTime(&race->sim, seat) : -1;
}
EMSCRIPTEN_KEEPALIVE int rs_seat_best_lap(int handle, int seat) {
    ServerRace *race = Race(handle);
    int best = -1;
    if (!race) return -1;
    if (seat < 0 || seat >= DRIVER_SEAT_LIMIT) return -1;
    /* Completed laps only: RaceLapTime also reports the lap in progress. */
    for (int lap = 0; lap < race->sim.laps && race->sim.drivers[seat].car.lap > lap + 1; ++lap) {
        const int time = RaceLapTime(&race->sim, seat, lap);
        if (time >= 0 && (best < 0 || time < best)) best = time;
    }
    return best;
}
EMSCRIPTEN_KEEPALIVE int rs_seat_lap(int handle, int seat) {
    ServerRace *race = Race(handle);
    return race && seat >= 0 && seat < DRIVER_SEAT_LIMIT ? race->sim.drivers[seat].car.lap : 0;
}
EMSCRIPTEN_KEEPALIVE int rs_seat_lap_time(int handle, int seat, int lap) {
    ServerRace *race = Race(handle);
    return race ? RaceLapTime(&race->sim, seat, lap) : -1;
}
/* Race progress (CarRaceProgress: laps included), as the ranking uses it. */
EMSCRIPTEN_KEEPALIVE int rs_seat_progress(int handle, int seat) {
    ServerRace *race = Race(handle);
    if (!race || seat < 0 || seat >= DRIVER_SEAT_LIMIT) return 0;
    const PlayerCarRuntime *car = &race->sim.drivers[seat].car;
    return (int)((uint32_t)car->progressA + (uint32_t)car->progressB);
}
/* World position of a seat's car (x, z), for checks of the starting grid. */
EMSCRIPTEN_KEEPALIVE int rs_seat_x(int handle, int seat) {
    ServerRace *race = Race(handle);
    return race && seat >= 0 && seat < DRIVER_SEAT_LIMIT ? race->sim.drivers[seat].car.x : 0;
}
EMSCRIPTEN_KEEPALIVE int rs_seat_z(int handle, int seat) {
    ServerRace *race = Race(handle);
    return race && seat >= 0 && seat < DRIVER_SEAT_LIMIT ? race->sim.drivers[seat].car.z : 0;
}
