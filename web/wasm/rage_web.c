/* WebAssembly bridge for the browser port (web/). It exposes a small, flat C
 * ABI over the same headless code the native modern renderer uses: disc
 * loading (rage-data), the race simulation (rage-sim), the client race and
 * scene submission (client_race.c, race_view.c, client_world.c) and the CPU
 * draw builder (render_mesh_build.c). The browser only uploads the resulting
 * world-space triangles and decoded textures; it never re-derives physics or
 * scene layout. */
#include <emscripten/emscripten.h>

#include <math.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "client_race.h"
#include "environment_view.h"
#include "game/car.h"
#include "game/race_data.h"
#include "game/race_grid.h"
#include "game/race_sim.h"
#include "game/asset_index.h"
#include "game/track.h"
#include "game/track_data.h"
#include "race_view.h"
#include "rage/chase_camera.h"
#include "render/car_lamps.h"
#include "render/render_mesh_build.h"
#include "render/render_projection.h"
#include "scene_matrix.h"

enum {
    WEB_INSTANCE_CAPACITY = 8192,
    WEB_VERTEX_CAPACITY = 600000,
    WEB_SPAN_CAPACITY = 32768,
    WEB_SPAN_FIELDS = 12,
    WEB_TEXTURE_BYTES = 256 * 256 * 4,
    WEB_COUNTDOWN_TICKS = 3 * SIM_TICK_RATE,
};

/* chase_camera.c reads optional tuning from the runtime config; the browser
 * has none, so every setting takes its built-in default. */
const char *RuntimeConfigGet(const char *key);
int RuntimeConfigEnabled(const char *key);
const char *RuntimeConfigGet(const char *key) { (void)key; return NULL; }
int RuntimeConfigEnabled(const char *key) { (void)key; return 0; }

static RaceData *s_archive;
static ClientRace *s_race;
static DriverInput s_input;
static int s_pendingShiftUp, s_pendingShiftDown;
static RenderMeshInstance *s_instances;
static RenderWorld s_world;
static RageNativeDrawVertex *s_vertices;
static RageNativeDrawSpan *s_spans;
static uint32_t s_vertexCount, s_spanCount;
static uint32_t s_spanFields[WEB_SPAN_CAPACITY * WEB_SPAN_FIELDS];
static uint64_t s_frame;
/* position, viewRow0, viewRow1, viewRow2, projection, fogColor, fogRange. */
static float s_camera[28];
/* direction, ambient, diffuse, skyTop, skyHorizon, skyBottom. */
static float s_light[24];
static int32_t s_hud[16];

static void ReleaseRace(void) {
    FreeClientRace(s_race);
    s_race = NULL;
}

EMSCRIPTEN_KEEPALIVE int rw_load_disc(const char *path) {
    ReleaseRace();
    FreeRaceData(s_archive);
    s_archive = LoadRaceDisc(path);
    return s_archive != NULL;
}

/* Frees the imported disc copy; a prepared race keeps its own data. */
EMSCRIPTEN_KEEPALIVE void rw_release_disc(void) {
    FreeRaceData(s_archive);
    s_archive = NULL;
}

EMSCRIPTEN_KEEPALIVE int rw_start_race(int classIndex, int course, int car, int manual,
                                       int reverse, int laps, int rivals) {
    RaceSetup setup;
    if (!s_archive || classIndex < 0 || classIndex >= TRACK_CLASS_COUNT || course < 0 ||
        course > 3 || car < 0 || car >= CAR_MODEL_VARIANT_COUNT || laps < 1 ||
        laps > PLAYER_LAP_TIME_CAPACITY) return 0;
    ReleaseRace();
    memset(&setup, 0, sizeof(setup));
    setup.classIndex = classIndex;
    setup.courseIndex = course;
    setup.laps = laps;
    setup.reverse = reverse ? 1 : 0;
    setup.entrants[0] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = 0, .model = car,
                                      .manual = manual ? 1 : 0, .seed = 0x5eed};
    setup.looks[0].variant = car;
    if (rivals) {
        /* Same field as the retail grid: every authored, active AI start,
         * with the final class limited to the contenders. */
        TrackData *track = CopyRaceTrack(s_archive, classIndex, course);
        if (!track) return 0;
        for (s32 seat = 1; seat < DRIVER_SEAT_LIMIT; ++seat) {
            if ((classIndex != TRACK_CLASS_COUNT - 1 || seat <= RIVAL_CONTENDER_COUNT) &&
                track->events->rivalStarts[setup.reverse][seat].activeFlag != -1)
                setup.entrants[seat] = (RaceEntrant){.kind = RACE_SEAT_AI, .grid = seat,
                                                     .model = seat - 1, .rivalSlot = seat - 1};
        }
        FreeTrackData(track);
    }
    s_race = LoadClientRace(s_archive, &setup, NULL);
    if (!s_race) return 0;
    if (!StartRaceSim(&s_race->sim, WEB_COUNTDOWN_TICKS)) {
        ReleaseRace();
        return 0;
    }
    if (!s_instances) s_instances = calloc(WEB_INSTANCE_CAPACITY, sizeof(*s_instances));
    if (!s_vertices) s_vertices = calloc(WEB_VERTEX_CAPACITY, sizeof(*s_vertices));
    if (!s_spans) s_spans = calloc(WEB_SPAN_CAPACITY, sizeof(*s_spans));
    if (!s_instances || !s_vertices || !s_spans) {
        ReleaseRace();
        return 0;
    }
    RenderWorldInit(&s_world, s_instances, WEB_INSTANCE_CAPACITY);
    memset(&s_input, 0, sizeof(s_input));
    s_input.steering.mode = STEERING_DIGITAL;
    s_pendingShiftUp = s_pendingShiftDown = 0;
    s_frame = 0;
    return 1;
}

/* Keyboard levels for the local seat. Gear requests are edges: they stay
 * pending until the next simulation tick consumes them. */
EMSCRIPTEN_KEEPALIVE void rw_set_input(int left, int right, int throttle, int brake,
                                       int shiftUp, int shiftDown) {
    s_input.steering.mode = STEERING_DIGITAL;
    s_input.steering.left = left ? 1 : 0;
    s_input.steering.right = right ? 1 : 0;
    s_input.steering.angle = 0;
    s_input.throttle = (s16)(throttle < 0 ? 0 : throttle > 256 ? 256 : throttle);
    s_input.brake = (s16)(brake < 0 ? 0 : brake > 256 ? 256 : brake);
    if (shiftUp) s_pendingShiftUp = 1;
    if (shiftDown) s_pendingShiftDown = 1;
}

static float Daylight(const ClientRace *race) {
    RenderCamera environment;
    memset(&environment, 0, sizeof(environment));
    ApplyEnvironment(&environment, &race->env);
    return CarLightDaylight(environment.skyTopColor, environment.skyHorizonColor);
}

/* One 50 Hz simulation tick. Returns the race phase, or -1 on failure. */
EMSCRIPTEN_KEEPALIVE int rw_tick(void) {
    DriverInput input;
    if (!s_race) return -1;
    input = s_input;
    input.shiftUp = s_pendingShiftUp;
    input.shiftDown = s_pendingShiftDown;
    if (SetRaceInput(&s_race->sim, 0, &input)) s_pendingShiftUp = s_pendingShiftDown = 0;
    StepRaceSim(&s_race->sim);
    if (!TickClientScenery(s_race)) return -1;
    TickRaceView(s_race->view, &s_race->sim, Daylight(s_race));
    return (int)s_race->sim.phase;
}

/* The native multiplayer/single-player chase rig, with the same host
 * adjustments; it reads the car, never mutates it. */
static RenderCamera BuildChaseCamera(const PlayerCarRuntime *car) {
    RenderCamera camera;
    float yawRad = AngleToDegrees(car->bodyYaw) * 0.017453292519943295f;
    float distance = (float)ChaseCameraDistance(2200);
    memset(&camera, 0, sizeof(camera));
    camera.transform.position.x = (float)car->x - sinf(yawRad) * distance;
    camera.transform.position.y = -(float)car->y + (float)ChaseCameraHeight(1400);
    camera.transform.position.z = -(float)car->z + cosf(yawRad) * distance;
    camera.transform.rotation.x = -22.0f - AngleToDegrees(ChaseCameraPitchOffset());
    camera.transform.rotation.y = -AngleToDegrees(car->bodyYaw) -
                                  AngleToDegrees(ChaseCameraYawOffset(car->steeringAngle));
    camera.transform.scale.x = camera.transform.scale.y = camera.transform.scale.z = 1.0f;
    camera.verticalFovDegrees = 65.0f;
    camera.nearPlane = 16.0f;
    camera.farPlane = 200000.0f;
    camera.fogNear = 60000.0f;
    camera.fogFar = 180000.0f;
    return camera;
}

/* Same rotation as modern_native_gpu.c's ModernNativeRotate. */
static void RotateByCamera(float out[3], const float in[3], const RenderCamera *camera) {
    float x = in[0], y = in[1], z = in[2];
    if (camera->transform.hasOrientation) {
        const Quaternion *q = &camera->transform.orientation;
        float length = sqrtf(q->x * q->x + q->y * q->y + q->z * q->z + q->w * q->w);
        if (length > 0.0f) {
            float qx = -q->x / length, qy = -q->y / length;
            float qz = -q->z / length, qw = q->w / length;
            float xx = qx * qx, yy = qy * qy, zz = qz * qz;
            float xy = qx * qy, xz = qx * qz, yz = qy * qz;
            float wx = qw * qx, wy = qw * qy, wz = qw * qz;
            out[0] = (1.0f - 2.0f * (yy + zz)) * x + 2.0f * (xy - wz) * y + 2.0f * (xz + wy) * z;
            out[1] = 2.0f * (xy + wz) * x + (1.0f - 2.0f * (xx + zz)) * y + 2.0f * (yz - wx) * z;
            out[2] = 2.0f * (xz - wy) * x + 2.0f * (yz + wx) * y + (1.0f - 2.0f * (xx + yy)) * z;
            return;
        }
    }
    {
        float rx = -camera->transform.rotation.x * 0.017453292519943295f;
        float ry = -camera->transform.rotation.y * 0.017453292519943295f;
        float rz = -camera->transform.rotation.z * 0.017453292519943295f;
        float c = cosf(rz), s = sinf(rz), next;
        next = x * c - y * s; y = x * s + y * c; x = next;
        c = cosf(ry); s = sinf(ry);
        next = x * c + z * s; z = -x * s + z * c; x = next;
        c = cosf(rx); s = sinf(rx);
        next = y * c - z * s; z = y * s + z * c; y = next;
    }
    out[0] = x; out[1] = y; out[2] = z;
}

/* Same uniform block as modern_native_gpu.c's ModernNativeBuildCamera. */
static int BuildCameraUniform(const RenderCamera *camera, float aspect) {
    static const float axes[3][3] = {{1, 0, 0}, {0, 1, 0}, {0, 0, 1}};
    float columns[3][3];
    memset(s_camera, 0, sizeof(s_camera));
    if (!RenderPerspectiveScales(camera, aspect, &s_camera[16], &s_camera[17]) ||
        !RenderPerspectiveDepthTerms(camera, &s_camera[18], &s_camera[19])) return 0;
    s_camera[0] = camera->transform.position.x;
    s_camera[1] = camera->transform.position.y;
    s_camera[2] = camera->transform.position.z;
    for (int axis = 0; axis < 3; ++axis) {
        RotateByCamera(columns[axis], axes[axis], camera);
        s_camera[4 + axis] = columns[axis][0];
        s_camera[8 + axis] = columns[axis][1];
        s_camera[12 + axis] = columns[axis][2];
    }
    s_camera[20] = camera->fogColor.x;
    s_camera[21] = camera->fogColor.y;
    s_camera[22] = camera->fogColor.z;
    if (isfinite(camera->fogNear) && isfinite(camera->fogFar) &&
        camera->fogNear > 0.0f && camera->fogFar > camera->fogNear) {
        s_camera[24] = camera->fogNear;
        s_camera[25] = camera->fogFar;
        s_camera[26] = 1.0f / camera->fogNear;
        s_camera[27] = s_camera[26] - 1.0f / camera->fogFar;
    }
    return 1;
}

static const RageRuntimeMesh *ResolveMesh(void *context, const RenderMeshInstance *instance) {
    const RageImportedMeshEntry *entry = FindClientMesh(context, instance);
    return entry ? &entry->cached.mesh : NULL;
}

static void StoreVec3(float *out, Vec3 value) {
    out[0] = value.x; out[1] = value.y; out[2] = value.z; out[3] = 0.0f;
}

/* Builds this frame's scene with the native sequence and expands it into
 * world-space triangles. Returns the vertex count, or -1 on failure. */
EMSCRIPTEN_KEEPALIVE int rw_build_frame(float aspect) {
    RenderDirectionalLight light;
    RenderCamera camera;
    const int page = 0;
    if (!s_race || !(aspect > 0.0f)) return -1;
    RenderWorldBeginFrame(&s_world, ++s_frame);
    camera = BuildChaseCamera(&s_race->sim.drivers[0].car);
    ApplyEnvironment(&camera, &s_race->env);
    RenderWorldSetCamera(&s_world, &camera);
    RenderDirectionalLightFromSky(&camera, &light);
    RenderWorldSetDirectionalLight(&s_world, &light);
    if (!SubmitClientTerrain(s_race, page, &s_world) ||
        !SubmitClientScenery(s_race, page, &s_world) ||
        !SubmitRaceView(&s_race->sim, s_race->view, s_race->rivals,
                        s_race->primaryMesh.cached.assetKey, 0, &s_world) ||
        !SubmitClientShuttles(s_race, page, &s_world) ||
        !SubmitClientSpinners(s_race, page, &s_world) ||
        !SubmitClientLandmarks(s_race, page, &s_world)) return -1;
    RenderWorldFocus(&s_world, 0);
    s_vertexCount = RenderBuildNativePassDraws(
        &s_world, RAGE_RENDER_PASS_MAIN, aspect, ResolveMesh, s_race,
        s_vertices, WEB_VERTEX_CAPACITY, s_spans, WEB_SPAN_CAPACITY, &s_spanCount);
    for (uint32_t i = 0; i < s_spanCount; ++i) {
        const RageNativeDrawSpan *span = &s_spans[i];
        uint32_t *out = &s_spanFields[i * WEB_SPAN_FIELDS];
        out[0] = span->firstVertex;
        out[1] = span->vertexCount;
        out[2] = span->material;
        out[3] = (uint32_t)span->assetSet;
        out[4] = (uint32_t)span->assetSource;
        out[5] = span->assetKey;
        out[6] = span->materialVariant;
        out[7] = span->hasCarPaint;
        out[8] = span->carPaintColor1;
        out[9] = span->carPaintColor2;
        out[10] = span->instanceFlags;
        out[11] = span->materialFlags;
    }
    if (!BuildCameraUniform(&s_world.camera, aspect)) return -1;
    StoreVec3(&s_light[0], s_world.light.direction);
    StoreVec3(&s_light[4], s_world.light.ambientColor);
    StoreVec3(&s_light[8], s_world.light.diffuseColor);
    StoreVec3(&s_light[12], s_world.camera.skyTopColor);
    StoreVec3(&s_light[16], s_world.camera.skyHorizonColor);
    StoreVec3(&s_light[20], s_world.camera.skyBottomColor);
    return (int)s_vertexCount;
}

EMSCRIPTEN_KEEPALIVE RageNativeDrawVertex *rw_vertices(void) { return s_vertices; }
EMSCRIPTEN_KEEPALIVE uint32_t *rw_spans(void) { return s_spanFields; }
EMSCRIPTEN_KEEPALIVE int rw_span_count(void) { return (int)s_spanCount; }
EMSCRIPTEN_KEEPALIVE int rw_span_fields(void) { return WEB_SPAN_FIELDS; }
EMSCRIPTEN_KEEPALIVE float *rw_camera(void) { return s_camera; }
EMSCRIPTEN_KEEPALIVE float *rw_light(void) { return s_light; }

/* Vertex layout, so the browser never hard-codes the C struct. */
EMSCRIPTEN_KEEPALIVE int rw_vertex_layout(int field) {
    switch (field) {
    case 0: return (int)sizeof(RageNativeDrawVertex);
    case 1: return (int)offsetof(RageNativeDrawVertex, position);
    case 2: return (int)offsetof(RageNativeDrawVertex, uv);
    case 3: return (int)offsetof(RageNativeDrawVertex, color);
    case 4: return (int)offsetof(RageNativeDrawVertex, normal);
    case 5: return (int)offsetof(RageNativeDrawVertex, fog);
    case 6: return (int)offsetof(RageNativeDrawVertex, lighting);
    case 7: return (int)offsetof(RageNativeDrawVertex, environmentLight);
    case 8: return (int)offsetof(RageNativeDrawVertex, depthBias);
    default: return -1;
    }
}

/* 256x256 RGBA for one span's material, exactly as the native backend
 * reconstructs it; the palette is the race's current environment CLUT. */
EMSCRIPTEN_KEEPALIVE int rw_decode_texture(int spanIndex, uint8_t *rgba) {
    RenderMeshInstance instance;
    const RageNativeDrawSpan *span;
    if (!s_race || spanIndex < 0 || (uint32_t)spanIndex >= s_spanCount || !rgba) return 0;
    span = &s_spans[spanIndex];
    if (span->material == UINT32_MAX) return 0;
    memset(&instance, 0, sizeof(instance));
    instance.assetKey = span->assetKey;
    instance.assetSet = span->assetSet;
    instance.assetSource = span->assetSource;
    instance.hasCarPaint = span->hasCarPaint;
    instance.carPaintColor1 = span->carPaintColor1;
    instance.carPaintColor2 = span->carPaintColor2;
    instance.materialVariant = span->materialVariant;
    return DecodeClientMaterial(s_race, &instance, span->material, 0, s_race->env.clut,
                                rgba, WEB_TEXTURE_BYTES);
}

/* Changes whenever the environment palette (time of day) changes, so the
 * browser knows when palette-dependent textures must be decoded again. */
EMSCRIPTEN_KEEPALIVE uint32_t rw_palette_hash(void) {
    uint32_t hash = 2166136261u;
    const uint8_t *bytes;
    if (!s_race) return 0;
    bytes = (const uint8_t *)s_race->env.clut;
    for (size_t i = 0; i < sizeof(s_race->env.clut); ++i) {
        hash ^= bytes[i];
        hash *= 16777619u;
    }
    return hash;
}

/* phase, countdown ticks left, lap, laps, place, entrants, race time ms,
 * speed (retail units), gear, status, finish place, tick, then the local
 * car's exact x, y, z and body yaw (used to check physics parity). */
EMSCRIPTEN_KEEPALIVE int32_t *rw_hud(void) {
    const SimDriver *driver;
    int entrants = 0;
    if (!s_race) return NULL;
    driver = &s_race->sim.drivers[0];
    for (int seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat)
        entrants += s_race->sim.drivers[seat].status != SIM_EMPTY;
    s_hud[0] = (int32_t)s_race->sim.phase;
    s_hud[1] = (int32_t)s_race->sim.countdown;
    s_hud[2] = driver->car.lap > s_race->sim.laps ? s_race->sim.laps : driver->car.lap;
    s_hud[3] = s_race->sim.laps;
    s_hud[4] = driver->place;
    s_hud[5] = entrants;
    s_hud[6] = RaceTime(&s_race->sim, 0);
    s_hud[7] = driver->car.speed;
    s_hud[8] = driver->car.drive.gear;
    s_hud[9] = (int32_t)driver->status;
    s_hud[10] = driver->place;
    s_hud[11] = (int32_t)s_race->sim.tick;
    s_hud[12] = driver->car.x;
    s_hud[13] = driver->car.y;
    s_hud[14] = driver->car.z;
    s_hud[15] = driver->car.bodyYaw;
    return s_hud;
}
