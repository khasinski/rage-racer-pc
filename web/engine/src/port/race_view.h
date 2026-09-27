#ifndef PORT_RACE_VIEW_H
#define PORT_RACE_VIEW_H
#include "game/race_sim.h"
#include "game/engine_sound.h"
#include "game/car_asset.h"
#include "game/race_data.h"
#include "game/track_look.h"
#include "render/car_lights.h"
#include "render/render_world.h"

enum { RACE_VIEW_CATCHUP_LIMIT = SIM_TICK_RATE * 10 };

typedef struct RaceCarLook {
    s32 variant;
    CarEntry paint;
    int hasPaint;
} RaceCarLook;

/* Owns copied human model/image sources, shared by variant within this view.
 * Course/AI assets remain the caller's responsibility. Do not copy by value. */
typedef struct RaceView {
    RaceCarLook looks[DRIVER_SEAT_LIMIT];
    CarModelData *models[CAR_MODEL_VARIANT_COUNT];
    EngineSound engines[DRIVER_SEAT_LIMIT];
    CarLights lamps[DRIVER_SEAT_LIMIT];
    GameCarRuntime cars[DRIVER_SEAT_LIMIT], previousCars[DRIVER_SEAT_LIMIT];
    u8 carSeen[DRIVER_SEAT_LIMIT];
    u32 tick;
    int tickSeen;
} RaceView;

/* NULL on missing models or mismatch with the simulation's known variant.
 * Inputs and existing views remain unchanged. */
RaceView *LoadRaceView(const RaceData *archive, const RaceSim *race,
                       const RaceCarLook looks[DRIVER_SEAT_LIMIT]);
void FreeRaceView(RaceView *view);

/* Update once after each simulation clock tick. Repeated rendering does not
 * advance pose history, engine or lamp presentation. daylight is the race's linear outdoor
 * brightness in [0,1]. Rewound ticks require a newly prepared view.
 * Returns 1 for an update, 0 for duplicate/older ticks or invalid field data. */
int TickRaceView(RaceView *view, const RaceSim *race, float daylight);

/* Publishes into caller-owned world storage; no game adapter or GPU callbacks.\n * Caller prepares the course and native assets before submission. Seat IDs
 * are renderer identities, independent of global player/AI storage. Invalid
 * visible-seat configuration rejects the entire submission before drawing.
 * rivals borrows placement records for logical AI slots; textureVariant is the
 * explicitly selected 0..31 track car palette pack, independent of local seat. */
int SubmitRaceView(const RaceSim *race, const RaceView *view,
                   const RivalLook *rivals, u32 trackAsset, u8 textureVariant, RenderWorld *world);
/* Optional presentation poses indexed by seat; does not mutate simulation.
 * Model identity and visibility still come from the authoritative field. */
int SubmitRaceViewPoses(const RaceSim *race, const RaceView *view,
                       const PlayerCarRuntime poses[DRIVER_SEAT_LIMIT],
                       const PlayerCarRuntime previous[DRIVER_SEAT_LIMIT],
                       const RivalLook *rivals, u32 trackAsset,
                       u8 textureVariant, RenderWorld *world);
#endif
