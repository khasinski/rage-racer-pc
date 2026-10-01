#ifndef PORT_CLIENT_RACE_H
#define PORT_CLIENT_RACE_H
#include "game/race_grid.h"
#include "race_view.h"
#include "native_mesh_writer.h"
#include "native_texture.h"
#include "game/terrain_bank.h"
#include "game/environment.h"
#include "game/track_look.h"
#include "game/course_objects.h"
#include "game/spinners.h"

/* Owns scene bytes borrowed by track/sim and the human model/image view. A prepared
 * race survives releasing its archive/configuration. Do not copy by value.
 * Owns runtime scene and texture bytes; GPU preparation remains separate. */
typedef struct ClientRace {
    unsigned references;
    TrackData track;
    NativeModelBank primary, secondary;
    CourseBank course;
    CourseObjects objects;
    TerrainBank terrain;
    Environment env;
    TrackLook look;
    RivalLook rivals[RACE_CAR_SLOT_COUNT];
    const GameEnvironmentScript *environment;
    const EnvironmentPalette *palettes;
    RageImportedMeshEntry primaryMesh, secondaryMesh, courseMesh, terrainMesh;
    RageImportedMeshEntry carMeshes[CAR_MODEL_VARIANT_COUNT];
    u32 carMeshCount;
    SceneAsset *scene;
    TrackImages *images;
    TrackPixels *pixels;
    RaceSim sim;
    RaceView *view;
    ShuttleConfig shuttlePaths[SHUTTLE_INSTANCE_COUNT];
    GameShuttleScenery shuttles[SHUTTLE_INSTANCE_COUNT];
    GameShuttleScenery previousShuttles[SHUTTLE_INSTANCE_COUNT];
    u32 sceneryTick, shuttleCount;
    int freezeScenery;
    Spinners spinners, previousSpinners;
    SpinningSceneryPlacement spinnerPlacements[4];
    StaticSceneryState landmarks;
    int ovalLandmark, highLandmark;
    u32 scenerySeed;
    int spinningScenery; /* 0 absent, 1 single, 2 group */
} ClientRace;

typedef struct RaceSetup {
    s32 classIndex, courseIndex, laps;
    int reverse;
    RaceEntrant entrants[DRIVER_SEAT_LIMIT];
    RaceCarLook looks[DRIVER_SEAT_LIMIT];
} RaceSetup;

/* Allocates a complete candidate or returns NULL without changing other races.
 * Start/input/step use the existing API on race->sim; no duplicated lifecycle. */
ClientRace *LoadClientRace(const RaceData *archive, const RaceSetup *setup,
                            const RageCarCatalog *catalog);
/* Main-thread lifetime: retained users keep all borrowed resource views alive.
 * Each successful retain requires one FreeClientRace. NULL/overflow rejects. */
ClientRace *RetainClientRace(ClientRace *race);
void FreeClientRace(ClientRace *race);
/* Borrowed prepared mesh, resolved only within this race. Keep a lifetime
 * reference while using it; no legacy/global fallback or implicit import. */
const RageImportedMeshEntry *FindClientMesh(const ClientRace *race,
                                           const RenderMeshInstance *instance);
/* Decode 256x256 RGBA from owned car/track sources, applying requested paint.
 * page is explicit 0/1; palette optionally borrows 16 captured environment
 * colors. Invalid input preserves output. Sources/output must not overlap. */
int DecodeClientMaterial(const ClientRace *race, const RenderMeshInstance *instance,
                          u32 material, int page, const u16 *palette,
                          u8 *rgba, size_t size);
/* Replaces owned main-pass terrain, preserving other instances. Authored
 * masks hide unrelated cells from rasterization, retaining ray geometry.
 * Invalid banks/page or insufficient space preserve the complete world. */
int SubmitClientTerrain(const ClientRace *race, int page, RenderWorld *world);
/* Replaces static owned course objects, preserving terrain/cars/dynamics.
 * Requires an explicit camera for wrapped-coordinate placement and visibility. */
int SubmitClientScenery(const ClientRace *race, int page, RenderWorld *world);
enum { CLIENT_SCENERY_CATCHUP_LIMIT = RACE_VIEW_CATCHUP_LIMIT };
/* A session resync (a page that stalled, a spectator catching up) animates
 * the scenery through up to half an hour of ticks at once: each is cheap. */
enum { CLIENT_SCENERY_RESYNC_LIMIT = SIM_TICK_RATE * 60 * 30 };
/* Advances to the simulation clock, including missed snapshots up to ten seconds.
 * Includes PAL environment cues at 25 Hz and scenery at the prototype clock.
 * Repeated ticks are inert; rewind/long gaps reject atomically. No physics runs. */
int TickClientScenery(ClientRace *race);
/* Brings the scenery to the race's tick however far it jumped (within
 * CLIENT_SCENERY_RESYNC_LIMIT), or back when it moved back. */
int ResyncClientScenery(ClientRace *race);
/* Publishes current/previous shuttle poses without advancing animation.
 * Replaces only this semantic entity range; failure preserves the world. */
int SubmitClientShuttles(const ClientRace *race, int page, RenderWorld *world);
int SubmitClientSpinners(const ClientRace *race, int page, RenderWorld *world);
int SubmitClientLandmarks(const ClientRace *race, int page, RenderWorld *world);
#endif
