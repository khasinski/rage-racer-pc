#include "mp_race_client.h"

#include "mp_client.h"
#include "client_race.h"
#include "client_frame.h"
#include "game/race_data.h"
#include "game/car_control.h"
#include "game/state.h"
#include "game/boot_internal.h"
#include "rage/compat.h"
#include "rage/chase_camera.h"
#include "rage/speed_display.h"
#include "scene_matrix.h"
#include "runtime_config.h"
#include "runtime_parse.h"
#include "host_disc.h"
#include "disc_discovery.h"
#include "platform_paths.h"
#include "environment_view.h"
#include "render/car_lamps.h"
#include "modern/modern_renderer.h"

#include <libetc.h>
#include <SDL3/SDL.h>

#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { MP_RACE_INSTANCE_CAPACITY = 8192 };

typedef struct MpView {
    MpHistory history;
    MpClock clock;
    PlayerCarRuntime cars[DRIVER_SEAT_LIMIT];
    uint64_t frame;
} MpView;

/* The native chase rig uses the same host adjustments as single-player,
 * without borrowing its global camera or mutating the authoritative car. */
static RenderCamera BuildChaseCamera(const PlayerCarRuntime *car) {
    RenderCamera camera;
    float yawRad = AngleToDegrees(car->bodyYaw) * 0.017453292519943295f;
    float distance = (float)ChaseCameraDistance(2200);
    float backX = sinf(yawRad) * distance;
    float backZ = -cosf(yawRad) * distance;

    memset(&camera, 0, sizeof(camera));
    camera.transform.position.x = (float)car->x - backX;
    camera.transform.position.y = -(float)car->y + (float)ChaseCameraHeight(1400);
    camera.transform.position.z = -(float)car->z - backZ;
    camera.transform.rotation.x = -22.0f - AngleToDegrees(ChaseCameraPitchOffset());
    camera.transform.rotation.y = -AngleToDegrees(car->bodyYaw) -
                                  AngleToDegrees(ChaseCameraYawOffset(car->steeringAngle));
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

static int RenderOneFrame(ClientRace *race, const PlayerCarRuntime *localCar, int localSeat, RenderWorld *world, MpView *view) {
    RenderDirectionalLight light;
    ClientFrame *frame;
    int ok;
    const int page = 0;
    const uint64_t now = SDL_GetTicksNS();
    PlayerCarRuntime poses[DRIVER_SEAT_LIMIT] = {0};
    for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) {
        poses[seat] = race->sim.drivers[seat].car;
        if (seat == localSeat) { poses[seat] = *localCar; continue; }
        MpCarPose pose;
        if (!MpHistoryPose(&view->history, seat, now, 100000000, &pose)) return 0;
        if (!MpApplyPose(&poses[seat], &pose, race->sim.drivers[seat].rival)) return 0;
    }

    RenderWorldBeginFrame(world, view->frame + 1);
    RenderCamera camera = BuildChaseCamera(&poses[localSeat]);
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
            const char *region = HostDiscRegion();
            const char *units = region && strcmp(region, "NTSC-U") == 0 ? "MPH" : "KPH";
            ok = MpHudText(&race->sim, localSeat,
                           SpeedDisplayValue(poses[localSeat].speed),
                           units, frame->hud) && ModernQueueClientFrame(frame);
            FreeClientFrame(frame);
        } else {
            ok = 0;
        }
    }
    if (ok) {
        for (int seat = 0; seat < MP_FIELD_LIMIT; ++seat) view->cars[seat] = poses[seat];
        view->frame++;
    }
    return ok;
}

/* Splits an edited "HOST:PORT" buffer in place; *host points inside it. */
static int ParseHostPort(char *hostPort, char **host, int *port) {
    char *colon = strrchr(hostPort, ':');
    int parsed;
    if (!colon || colon == hostPort || !RuntimeParseInt(colon + 1, 10, 1, 65535, &parsed))
        return 0;
    *colon = '\0';
    *host = hostPort;
    *port = parsed;
    return 1;
}

static int LoadSavedText(const char *fileName, char *text, size_t capacity) {
    char path[PATH_MAX];
    return PlatformUserConfigPath(fileName, path, sizeof(path)) &&
           DiscReadSavedPath(path, text, capacity);
}

static void SaveText(const char *fileName, const char *text) {
    char directory[PATH_MAX], path[PATH_MAX];
    if (!PlatformUserConfigDirectory(directory, sizeof(directory)) ||
        !PlatformEnsureDirectory(directory)) return;
    if (PlatformUserConfigPath(fileName, path, sizeof(path))) DiscWriteSavedPath(path, text);
}

int PortRunMultiplayer(int interactive) {
    const char *host = RuntimeConfigGet("multiplayer.connect_host");
    const char *name = RuntimeConfigGet("multiplayer.connect_name");
    MpSettings settings;
    uint64_t roomCode;
    if (!MpParseSettings(RuntimeConfigGet("multiplayer.connect_port"),
                         RuntimeConfigGet("multiplayer.connect_car"),
                         RuntimeConfigGet("multiplayer.connect_manual"), &settings) ||
        !MpParseRoom(RuntimeConfigGet("multiplayer.connect_room"), &roomCode)) {
        fprintf(stderr, "rage-port: multiplayer: invalid port, room, car or transmission setting\n");
        if (interactive) MpShowError("INVALID CONNECTION SETTINGS");
        return 0;
    }
    RaceData *archive = NULL;
    MpClient *client = NULL;
    int seat = -1;
    MpStart start;
    RaceSetup setup;
    ClientRace *race = NULL;
    RaceSim *prediction = NULL;
    RenderMeshInstance *instances = NULL;
    int success = 0;
    const char *error = "COULD NOT CONNECT TO SERVER";
    const int restoreClassic = !ModernIsEnabled();
    const int hostGiven = host != NULL && host[0] != '\0';
    const int nameGiven = name != NULL && name[0] != '\0';
    char nameBuffer[MP_NAME_CAPACITY + 1] = {0};
    char serverBuffer[32] = {0};

    if (!hostGiven) host = "127.0.0.1";
    if (!nameGiven) name = "Player";

    /* VSync creates the host window lazily. Verify it before opening a
     * network session or reporting loaded; input setup is shared with boot. */
    VSync(0);
    if (restoreClassic) ModernToggle();
    if (!ModernPrepareClientPresentation()) {
        error = "RENDERER COULD NOT INITIALIZE";
        fprintf(stderr, "rage-port: multiplayer: modern presentation could not initialize\n");
        goto cleanup;
    }

    /* A command-line override (used by scripted/non-interactive runs) skips
     * these; an interactive player without one always types both. */
    if (interactive && !nameGiven) {
        int entered;
        LoadSavedText("rage-mp-name.cfg", nameBuffer, sizeof(nameBuffer));
        entered = MpEnterName(nameBuffer, sizeof(nameBuffer));
        if (entered != 1) { success = entered == -1; goto cleanup; }
        SaveText("rage-mp-name.cfg", nameBuffer);
        name = nameBuffer;
    }
    if (interactive && !hostGiven) {
        char *parsedHost;
        int parsedPort, entered;
        if (!LoadSavedText("rage-mp-server.cfg", serverBuffer, sizeof(serverBuffer)))
            snprintf(serverBuffer, sizeof(serverBuffer), "127.0.0.1:7243");
        entered = MpEnterServer(serverBuffer, sizeof(serverBuffer));
        if (entered != 1) { success = entered == -1; goto cleanup; }
        SaveText("rage-mp-server.cfg", serverBuffer);
        if (!ParseHostPort(serverBuffer, &parsedHost, &parsedPort)) {
            error = "INVALID SERVER ADDRESS";
            goto cleanup;
        }
        host = parsedHost;
        settings.port = parsedPort;
    }

    fprintf(stderr, "rage-port: multiplayer connecting to %s:%d as '%s'\n", host, settings.port, name);
    archive = HostCopyRaceData();
    if (!archive) {
        error = "COULD NOT READ DISC DATA";
        fprintf(stderr, "rage-port: multiplayer: failed to copy the mounted disc data\n");
        goto cleanup;
    }
    if (interactive) {
        int roomSelected = MpSelectRoom(&roomCode);
        if (roomSelected != 1) {
            error = "INVALID ROOM CODE";
            success = roomSelected == -1;
            goto cleanup;
        }
    }

    client = MpClientBeginConnect(host, (uint16_t)settings.port);
    MpCarConfig config;
    uint32_t automaticCars;
    enum { CONNECT, HELLO, BROWSE, ROOM, WELCOME, AVAILABILITY, OPTIONS, PICK, READY, START, CONFIG, JOINED };
    MpRaceOptions options = {.laps = 3};
    int cancelled = 0;
    int phase = CONNECT;
    while (client && phase < JOINED) {
        if (interactive) {
            char roomLabel[48];
            uint64_t assigned = MpClientRoom(client);
            if (assigned) snprintf(roomLabel, sizeof(roomLabel), "ROOM %llu / %s",
                                   (unsigned long long)assigned, seat == 0 ? "HOST" : "GUEST");
            else snprintf(roomLabel, sizeof(roomLabel), "CAR CONFIRMED");
            DrawHostMenuFrame("MULTIPLAYER", roomLabel,
                             phase < START ? "CONNECTING..." : "WAITING FOR OTHER PLAYER",
                             "CANCEL: RETURN TO MENU");
        } else {
            VSync(0);
            PortSampleAnalogPad();
            UpdatePadState();
        }
        if (g_PadPressed & PAD_CANCEL) { cancelled = 1; break; }
        int status;
        switch (phase) {
        case CONNECT: status = MpClientPollConnect(client); break;
        case HELLO: status = MpClientPollHello(client, name); break;
        case BROWSE:
            status = interactive && roomCode == MP_BROWSE_ROOM ? MpBrowseRooms(client, &roomCode) : 1;
            if (status == -1) { cancelled = 1; status = 0; }
            break;
        case ROOM: status = MpClientPollRoom(client, roomCode); break;
        case WELCOME: status = MpClientPollWelcome(client, &seat); break;
        case AVAILABILITY:
            status = MpClientPollAvailability(client, &automaticCars);
            if (status == 1) {
                if (interactive) {
                    int selected = MpSelectCar(&settings, archive, automaticCars);
                    if (selected != 1) { cancelled = selected == -1; status = 0; }
                } else if (!settings.manual && !(automaticCars & (UINT32_C(1) << settings.car))) status = 0;
            }
            break;
        case OPTIONS:
            status = interactive && seat == 0 ? MpClientPollRace(client, &options) : 1;
            break;
        case PICK: status = MpClientPollPick(client, settings.car, settings.manual); break;
        case CONFIG: status = MpClientPollConfig(client, &config); break;
        case READY:
            status = interactive ? MpWaitRoom(client, seat, &start, &settings, automaticCars) : MpClientPollReady(client, 1);
            if (status == -1) { cancelled = 1; status = 0; }
            if (status == 1 && interactive) phase = START;
            break;
        default: status = MpClientPollStart(client, &start); break;
        }
        if (!status) break;
        if (status == 1) {
            phase++;
            if (phase == OPTIONS && interactive && seat == 0) {
                int selected = MpSelectRace(&options);
                if (selected != 1) { cancelled = selected == -1; break; }
            }
        }
    }
    if (phase != JOINED) {
        error = phase == CONNECT ? "COULD NOT CONNECT TO SERVER" : "SERVER HANDSHAKE FAILED";
        fprintf(stderr, "rage-port: multiplayer: %s\n",
                cancelled ? "connection cancelled" : "handshake with server failed");
        success = cancelled;
        goto cleanup;
    }
    fprintf(stderr, "rage-port: multiplayer: joined as seat %d, %d laps\n", seat, start.laps);

    if (!MpMatchesChoice(&start, seat, &settings)) {
        error = "SERVER CHANGED YOUR CAR CHOICE";
        fprintf(stderr, "rage-port: multiplayer: server changed the selected car or transmission\n");
        goto cleanup;
    }

    if (!MpMatchesArchive(&start, archive)) {
        error = "SERVER AND CLIENT DISCS DIFFER";
        fprintf(stderr, "rage-port: multiplayer: server and client discs do not match\n");
        goto cleanup;
    }

    if (!MpBuildSetup(&start, &setup)) {
        error = "INVALID RACE SETUP";
        goto cleanup;
    }
    
    race = LoadClientRace(archive, &setup, NULL);
    if (!race) {
        error = "COULD NOT LOAD RACE DATA";
        fprintf(stderr, "rage-port: multiplayer: failed to build the race\n");
        goto cleanup;
    }

    if (!MpApplyConfig(&race->sim, &config)) {
        error = "INVALID SERVER CAR CONFIGURATION";
        goto cleanup;
    }
    if (!StartRaceSim(&race->sim, start.countdown)) {
        error = "COULD NOT START RACE";
        goto cleanup;
    }

    instances = calloc(MP_RACE_INSTANCE_CAPACITY, sizeof(*instances));
    if (!instances) {
        error = "NOT ENOUGH MEMORY FOR RACE";
        goto cleanup;
    }
    RenderWorld world;
    RenderWorldInit(&world, instances, MP_RACE_INSTANCE_CAPACITY);
    /* The prepared race owns its data; the import archive is no longer needed. */
    FreeRaceData(archive);
    archive = NULL;

    int loaded;
    do {
        VSync(0);
        PortSampleAnalogPad();
        UpdatePadState();
        if (g_PadPressed & PAD_CANCEL) { cancelled = 1; loaded = 0; break; }
        loaded = MpClientPollLoaded(client);
    } while (loaded == 3);
    if (!loaded) {
        error = "COULD NOT REPORT RACE LOADED";
        success = cancelled;
        goto cleanup;
    }

    prediction = malloc(2 * sizeof(*prediction));
    if (!prediction) { error = "COULD NOT ALLOCATE PREDICTION"; goto cleanup; }
    MpView view = {.history = {.receivedAt = SDL_GetTicksNS()}};
    for (;;) {
        VSync(0);
        PortSampleAnalogPad();
        UpdatePadState();
        if ((g_PadHeld & (PAD_START | PAD_SELECT)) == (PAD_START | PAD_SELECT) &&
            (g_PadPressed & (PAD_START | PAD_SELECT))) {
            success = 1;
            break;
        }
        DriverInput input = ReadCarControls();
        MpSnapshot snapshot;
        MpResult result;
        int received;

        received = MpClientPollRaceState(client, &race->sim, &snapshot, &result, NULL);
        if (received == 0) {
            error = "SERVER DISCONNECTED OR BAD PACKET";
            fprintf(stderr, "rage-port: multiplayer: server disconnected\n");
            break;
        }
        if (received == 2) {
            if (!view.history.count || !MpMatchesResult(
                    &view.history.samples[view.history.count - 1], &result)) {
                fprintf(stderr, "rage-port: multiplayer: result disagrees with final race state\n");
                error = "RESULT DOES NOT MATCH RACE";
                break;
            }
            success = 1;
            fprintf(stderr, "rage-port: multiplayer: race finished, place=%d time=%d ms\n", result.seats[seat].place, result.seats[seat].milliseconds);
            MpClientClose(client);
            client = NULL;
            if (interactive) {
                ModernQueueClientFrame(NULL);
                MpShowResults(&result, seat);
            }
            break;
        }
        if (received == 3 && MpSnapshotExpired(SDL_GetTicksNS(), view.history.receivedAt, view.history.count != 0)) {
            error = "SERVER STOPPED SENDING RACE STATE";
            fprintf(stderr, "rage-port: multiplayer: server stopped sending race state\n");
            break;
        }
        if (received == 1 || received == 4) {
            if (!MpClockObserve(&view.clock, snapshot.tick, SDL_GetTicksNS())) {
                error = "INVALID PREDICTION CLOCK";
                break;
            }
            if (received == 1 && !MpApplySnapshot(&race->sim, &snapshot)) {
                error = "INVALID OR STALE RACE STATE";
                fprintf(stderr, "rage-port: multiplayer: invalid or stale snapshot\n");
                break;
            }
            if (!TickClientScenery(race)) {
                error = "SCENERY CLOCK OUT OF SYNC";
                fprintf(stderr, "rage-port: multiplayer: scenery clock needs resync\n");
                break;
            }
            RenderCamera environment = {0};
            ApplyEnvironment(&environment, &race->env);
            float daylight = CarLightDaylight(environment.skyTopColor, environment.skyHorizonColor);
            if (!TickRaceView(race->view, &race->sim, daylight)) {
                error = "INVALID VEHICLE PRESENTATION";
                fprintf(stderr, "rage-port: multiplayer: invalid vehicle presentation\n");
                break;
            }
            if (!MpHistoryPush(&view.history, &snapshot, SDL_GetTicksNS())) {
                error = "INVALID SNAPSHOT HISTORY";
                break;
            }
        }
        /* Read final state/results before writing: the server closes its
         * connection after draining the result. A final snapshot may defer
         * that result until the next poll, so stop inputs once it finishes. */
        uint32_t inputTick = 0, fraction = 0;
        if (view.clock.started && !MpClockTarget(&view.clock, SDL_GetTicksNS(), &inputTick, &fraction)) {
            error = "INVALID INPUT CLOCK";
            break;
        }
        if (race->sim.phase != SIM_FINISHED && race->sim.drivers[seat].status == SIM_DRIVING &&
            !MpClientPollInput(client, &input, inputTick)) {
            error = "COULD NOT SEND DRIVER INPUT";
            break;
        }
        if (!view.history.count) continue;
        MpCommand pending[2];
        unsigned pendingCount = MpClientPendingCommands(client, pending);
        if (!MpPredictRace(&race->sim, MpClientCommands(client), pending, pendingCount,
                           seat, inputTick, &prediction[0])) {
            error = "COULD NOT PREDICT LOCAL CAR";
            break;
        }
        PlayerCarRuntime localCar = prediction[0].drivers[seat].car;
        uint32_t nextTick;
        if (!MpMotionTarget(&prediction[0], race->sim.tick, seat, fraction, &nextTick, &fraction)) {
            error = "INVALID MOTION CLOCK";
            break;
        }
        if (fraction) {
            if (!MpPredictRace(&race->sim, MpClientCommands(client), pending, pendingCount,
                               seat, nextTick, &prediction[1]) ||
                !MpBlendDriver(&prediction[0].drivers[seat], &prediction[1].drivers[seat], fraction, &localCar)) {
                error = "COULD NOT INTERPOLATE LOCAL CAR";
                break;
            }
        }
        /* Show the remote seat with a short history delay, without
         * extrapolating during stalls or advancing authoritative state. */
        if (!RenderOneFrame(race, &localCar, seat, &world, &view)) {
            error = "FRAME SUBMISSION FAILED";
            fprintf(stderr, "rage-port: multiplayer: frame submission failed\n");
            break;
        }
    }

cleanup:
    ModernQueueClientFrame(NULL);
    free(prediction);
    free(instances);
    FreeClientRace(race);
    MpClientClose(client);
    FreeRaceData(archive);
    if (restoreClassic && ModernIsEnabled()) ModernToggle();
    if (!success && interactive) MpShowError(error);
    return success;
}
