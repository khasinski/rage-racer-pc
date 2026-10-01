#ifndef GAME_RACE_GRID_H
#define GAME_RACE_GRID_H
#include "game/race_data.h"
#include "game/race_sim.h"
#include "game/car_catalog.h"

typedef enum RaceSeatKind { RACE_SEAT_EMPTY, RACE_SEAT_HUMAN, RACE_SEAT_AI } RaceSeatKind;
typedef struct RaceEntrant {
    RaceSeatKind kind;
    s32 grid;       /* Authored starting position, independent of field seat. */
    s32 model;      /* Human retail variant, or logical AI model. */
    s32 rivalSlot;  /* Authored AI behavior slot. */
    s16 manual;
    /* Retail tire compound, 0..4. The showroom stores it in the same word as
     * the launch-threshold index, and that is what the simulation reads. */
    s32 tire;
    u32 seed;
    /* Web port: a human may start from an explicit place instead of the
     * authored start at grid (hasStart set; activeFlag 0, as the player). */
    int hasStart;
    TrackRivalStart start;
} RaceEntrant;

/* Borrow track data for the resulting race's lifetime. Specifications and
 * catalog overrides are copied per human; no archive/catalog borrow remains.
 * Empty entries stay empty; an explicitly requested inactive AI is rejected.
 * Construction is atomic, including failure on a late entrant. */
int InitRaceGrid(RaceSim *race, const RaceData *archive, const TrackData *track,
                  const RaceEntrant entrants[DRIVER_SEAT_LIMIT],
                  const RageCarCatalog *catalog, s32 laps, int reverse);
#endif
