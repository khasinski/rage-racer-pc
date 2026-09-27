#include "mp_race_client.h"

#include "mp_client.h"
#include "client_race.h"
#include "client_frame.h"
#include "game/race_data.h"
#include "game/car_control.h"
#include "game/state.h"
#include "rage/compat.h"
#include "scene_matrix.h"
#include "runtime_config.h"
#include "environment_view.h"
#include "render/car_lamps.h"
#include "modern/modern_renderer.h"

#include <libetc.h>
#include <SDL3/SDL.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MP_RACE_INSTANCE_CAPACITY = 8192 };

typedef struct MpView {
    MpSnapshot before, after;
    PlayerCarRuntime cars[DRIVER_SEAT_LIMIT];
    uint64_t receivedAt, frame;
    int seen;
} MpView;

static const char *ResolveDiscPath(void) {
    const char *path = RuntimeConfigGet("disc.image");
    if (path == NULL || path[0] == '\0') path = RuntimeConfigGetForced("disc.cue");
    if (path == NULL || path[0] == '\0')
        path = "disc/PAL/Rage Racer (Europe)/Rage Racer (Europe).cue";
    return path;
}

/* Fixed, world-axis-aligned high chase camera above and behind the local
 * seat's car, facing the direction it is heading. Good enough to show both
 * cars racing without reproducing the retail attached-rig camera matrix
 * (see docs/multiplayer.md step 3: draw the other car, nothing more). */
static RenderCamera BuildChaseCamera(const PlayerCarRuntime *car) {
    RenderCamera camera;
    float yawRad = AngleToDegrees(car->bodyYaw) * 0.017453292519943295f;
    float backX = sinf(yawRad) * 2200.0f;
    float backZ = -cosf(yawRad) * 2200.0f;

    memset(&camera, 0, sizeof(camera));
    camera.transform.position.x = (float)car->x - backX;
    camera.transform.position.y = -(float)car->y + 1400.0f;
    camera.transform.position.z = -(float)car->z - backZ;
    camera.transform.rotation.x = -22.0f;
    camera.transform.rotation.y = -AngleToDegrees(car->bodyYaw);
    camera.transform.rotation.z = 0.0f;
    camera.transform.scale.x = camera.transform.scale.y = camera.transform.scale.z = 1.0f;
    camera.transform.hasOrientation = 0;
    camera.verticalFovDegrees = 65.0f;
    camera.nearPlane = 16.0f;
    camera.farPlane = 200000.0f;
    camera.fogNear = 60000.0f;
    camera.fogFar = 180000.0f;
    return camera;
}

static int RenderOneFrame(ClientRace *race, int localSeat, RenderWorld *world, MpView *view) {
    RenderDirectionalLight light;
    ClientFrame *frame;
    int ok;
    const int page = 0;
    uint64_t duration = (uint64_t)(view->after.tick - view->before.tick) * 1000000000 / SIM_TICK_RATE;
    uint64_t elapsed = SDL_GetTicksNS() - view->receivedAt;
    uint32_t fraction = !duration || elapsed >= duration ? 65536 :
                        (uint32_t)(elapsed * 65536 / duration);
    PlayerCarRuntime poses[DRIVER_SEAT_LIMIT] = {0};
    for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) {
        poses[seat] = race->sim.drivers[seat].car;
        if (seat == localSeat) continue; /* Prediction is a separate step. */
        MpCarPose pose;
        if (!MpBlendPose(&view->before.seats[seat], &view->after.seats[seat], fraction, &pose)) return 0;
        poses[seat].x = pose.x;
        poses[seat].y = pose.y;
        poses[seat].z = pose.z;
        poses[seat].bodyYaw = pose.yaw;
        poses[seat].bodyPitch = pose.pitch;
        poses[seat].bodyRoll = pose.roll;
        poses[seat].steeringAngle = pose.steering;
        poses[seat].wheelRotation = pose.wheels;
        poses[seat].modelY = pose.ground;
        poses[seat].bodyRollVelocity = pose.rollSpeed;
    }

    RenderWorldBeginFrame(world, view->frame + 1);
    RenderCamera camera = BuildChaseCamera(&race->sim.drivers[localSeat].car);
    ApplyEnvironment(&camera, &race->env);
    RenderWorldSetCamera(world, &camera);
    RenderDirectionalLightFromSky(&camera, &light);
    RenderWorldSetDirectionalLight(world, &light);

    ok = SubmitClientTerrain(race, page, world) &&
         SubmitClientScenery(race, page, world) &&
         SubmitRaceViewPoses(&race->sim, race->view, poses,
                            view->frame ? view->cars : poses, race->rivals,
                        race->primaryMesh.cached.assetKey, page, world) &&
         SubmitClientShuttles(race, page, world) && SubmitClientSpinners(race, page, world) &&
         SubmitClientLandmarks(race, page, world);
    if (ok) {
        /* The local seat can be either participant, regardless of model bank. */
        RenderWorldFocus(world, (uint32_t)localSeat);
        frame = CaptureClientFrame(race, world, page);
        if (frame) {
            ok = ModernQueueClientFrame(frame);
            FreeClientFrame(frame);
        } else {
            ok = 0;
        }
    }
    if (ok) {
        for (int seat = 0; seat < MP_SEAT_LIMIT; ++seat) view->cars[seat] = poses[seat];
        view->frame++;
    }
    return ok;
}

int RunMultiplayerRaceClient(const RagePortConfig *config) {
    const char *discPath = ResolveDiscPath();
    const char *host = RuntimeConfigGet("multiplayer.connect_host");
    const char *name = RuntimeConfigGet("multiplayer.connect_name");
    int port = RuntimeConfigInt("multiplayer.connect_port", 7878, 1, 65535);
    (void)config;
    RaceData *archive;
    MpClient *client;
    int seat = -1;
    MpStart start;
    RaceSetup setup;
    ClientRace *race;
    int success = 0;

    if (host == NULL || host[0] == '\0') host = "127.0.0.1";
    if (name == NULL || name[0] == '\0') name = "Player";

    /* VSync creates the host window lazily. Verify it before opening a
     * network session or reporting loaded; input setup is shared with boot. */
    GameInitPad();
    VSync(0);
    if (!ModernPrepareClientPresentation()) {
        fprintf(stderr, "rage-port: multiplayer: modern presentation could not initialize\n");
        return 0;
    }

    fprintf(stderr, "rage-port: multiplayer connecting to %s:%d as '%s'\n", host, port, name);
    archive = LoadRaceDisc(discPath);
    if (!archive) {
        fprintf(stderr, "rage-port: multiplayer: failed to load disc at %s\n", discPath);
        return 0;
    }

    client = MpClientConnect(host, (uint16_t)port);
    if (!client) {
        fprintf(stderr, "rage-port: multiplayer: could not connect to %s:%d\n", host, port);
        FreeRaceData(archive);
        return 0;
    }
    if (!MpClientSendHello(client, name) || !MpClientRecvWelcome(client, &seat) ||
        !MpClientRecvStart(client, &start)) {
        fprintf(stderr, "rage-port: multiplayer: handshake with server failed\n");
        MpClientClose(client);
        FreeRaceData(archive);
        return 0;
    }
    fprintf(stderr, "rage-port: multiplayer: joined as seat %d, %d laps\n", seat, start.laps);

    if (!MpMatchesArchive(&start, archive)) {
        fprintf(stderr, "rage-port: multiplayer: server and client discs do not match\n");
        MpClientClose(client);
        FreeRaceData(archive);
        return 0;
    }

    if (!MpBuildSetup(&start, &setup)) {
        MpClientClose(client);
        FreeRaceData(archive);
        return 0;
    }
    
    race = LoadClientRace(archive, &setup, NULL);
    if (!race) {
        fprintf(stderr, "rage-port: multiplayer: failed to build the race\n");
        MpClientClose(client);
        FreeRaceData(archive);
        return 0;
    }

    if (!StartRaceSim(&race->sim, start.countdown)) {
        FreeClientRace(race);
        MpClientClose(client);
        FreeRaceData(archive);
        return 0;
    }

    RenderMeshInstance *instances = calloc(MP_RACE_INSTANCE_CAPACITY, sizeof(*instances));
    if (!instances) {
        FreeClientRace(race);
        MpClientClose(client);
        FreeRaceData(archive);
        return 0;
    }
    RenderWorld world;
    RenderWorldInit(&world, instances, MP_RACE_INSTANCE_CAPACITY);
    /* The prepared race owns its data; the import archive is no longer needed. */
    FreeRaceData(archive);

    if (!MpClientSendLoaded(client)) {
        free(instances);
        FreeClientRace(race);
        MpClientClose(client);
        return 0;
    }

    MpView view = {0};
    for (;;) {
        VSync(0);
        PortSampleAnalogPad();
        UpdatePadState();
        DriverInput input = ReadCarControls();
        MpSnapshot snapshot;
        MpResult result;
        int received;

        if (!MpClientPollInput(client, &input)) break;
        received = MpClientPollMessage(client, &snapshot, &result);
        if (received == 0) {
            fprintf(stderr, "rage-port: multiplayer: server disconnected\n");
            break;
        }
        if (received == 2) {
            success = 1;
            fprintf(stderr, "rage-port: multiplayer: race finished, place=%d time=%d ms\n", result.seats[seat].place, result.seats[seat].milliseconds);
            break;
        }
        if (received == 1) {
            if (!MpApplySnapshot(&race->sim, &snapshot)) {
                fprintf(stderr, "rage-port: multiplayer: invalid or stale snapshot\n");
                break;
            }
            if (!TickClientScenery(race)) {
                fprintf(stderr, "rage-port: multiplayer: scenery clock needs resync\n");
                break;
            }
            RenderCamera environment = {0};
            ApplyEnvironment(&environment, &race->env);
            float daylight = CarLightDaylight(environment.skyTopColor, environment.skyHorizonColor);
            if (!TickRaceView(race->view, &race->sim, daylight)) {
                fprintf(stderr, "rage-port: multiplayer: invalid vehicle presentation\n");
                break;
            }
            view.before = view.seen ? view.after : snapshot;
            view.after = snapshot;
            view.receivedAt = SDL_GetTicksNS();
            view.seen = 1;
        }
        if (!view.seen) continue;
        /* Show the remote seat one snapshot interval behind, without
         * extrapolating during stalls or advancing authoritative state. */
        if (!RenderOneFrame(race, seat, &world, &view)) {
            fprintf(stderr, "rage-port: multiplayer: frame submission failed\n");
            break;
        }
    }

    ModernQueueClientFrame(NULL);
    free(instances);
    FreeClientRace(race);
    MpClientClose(client);
    return success;
}
