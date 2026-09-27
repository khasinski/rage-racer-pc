#include "game/track_look.h"
#include "environment_view.h"
#include "game/angle.h"
#include "rage/render_world_game.h"
#include "rage/render_world_scene.h"

#include "course_coordinate.h"
#include "car_parts.h"
#include "sky_panorama_layout.h"
#include "runtime_config.h"
#include "native_visibility.h"
#include "timing_control.h"

#include <math.h>
#include <stdio.h>
#include "render/car_lamps.h"
#include <stdlib.h>
#include <string.h>

#include "game/asset.h"
#include "game/car_asset.h"
#include "game/asset_index.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "render/render_world_frame.h"
#include "render/car_paint.h"
#include "modern/scene_capture.h"
#include "rage/track_asset_identity.h"
#include "render/track_lighting.h"

enum { RAGE_GAME_RENDER_WORLD_MAX_INSTANCES = 4096 };
static const float START_GRID_DEPTH_BIAS = -2048.0f;

static RenderMeshInstance s_instances[3][RAGE_GAME_RENDER_WORLD_MAX_INSTANCES];
static RenderWorld s_worlds[3];
static RenderMeshInstance
    s_presentationInstances[RAGE_GAME_RENDER_WORLD_MAX_INSTANCES];
static RenderWorld s_presentationWorld;
static uint64_t s_presentationSerial;
enum {
    RAGE_CAR_RENDER_PART_COUNT = 6,
    RAGE_PLAYER_CAR_ENTITY = RACE_CAR_SLOT_COUNT,
    RAGE_CAR_ENTITY_COUNT = RACE_CAR_SLOT_COUNT + 1
};
static RenderTransform
    s_previousCars[RAGE_CAR_ENTITY_COUNT][RAGE_CAR_RENDER_PART_COUNT];
static uint8_t
    s_havePreviousCars[RAGE_CAR_ENTITY_COUNT][RAGE_CAR_RENDER_PART_COUNT];
/* Per-frame presentation inputs copied from the submitted car, never borrowed
 * from a global player/traffic array at publication time. */
static struct {
    float zoneDaylight;
    int braking;
    int present;
} s_carLightInputs[RAGE_CAR_ENTITY_COUNT];

static void CaptureCarLightInput(uint32_t entity, const GameCarRuntime *car, const TrackZoneEffect *effect) {
    if (entity >= RAGE_CAR_ENTITY_COUNT) return;
    TrackZoneEffect zone = effect ? *effect : GetTrackZoneEffect(car->trackProgress);
    s_carLightInputs[entity].zoneDaylight = TrackZoneDaylight(zone.blend);
    s_carLightInputs[entity].braking = car->brakeInput > 0;
    s_carLightInputs[entity].present = 1;
}

static int s_trackCarAsset = -1;
static int s_initialized;
static int s_buildingIndex;
static int s_publishedWorld = 1, s_previousWorld = 2;
static int s_publishedCount, s_buildingWorld;
static int s_verifyPublication = -1;
static uint64_t s_publishedHash, s_previousHash;

static uint64_t WorldPublicationHash(const RenderWorld *world) {
    uint64_t hash = UINT64_C(0xcbf29ce484222325);
    const unsigned char *bytes = (const unsigned char *)world;
    for (size_t i = 0; i < sizeof(*world); ++i)
        hash = (hash ^ bytes[i]) * UINT64_C(1099511628211);
    bytes = (const unsigned char *)world->instances;
    for (size_t i = 0; i < world->instanceCount * sizeof(*world->instances); ++i)
        hash = (hash ^ bytes[i]) * UINT64_C(1099511628211);
    return hash;
}
static GameSkyGridLayout s_skyGrid[2];
static int s_haveSkyGrid[2];

static int GameSceneUsesRaceWorld(void) {
    return GameRenderWorldSceneHas3d((GameSceneId)g_SceneId);
}

static RenderWorld *GameRenderWorldMutable(void) {
    return &s_worlds[s_buildingIndex];
}

/* Menus and race-result screens are captured 2D scenes.  They must not
 * inherit the last race camera: doing so made the modern path keep drawing a
 * moving sky and track behind the Lost Race prompt after the game had stopped
 * producing race-world packets. */
static void GameRenderWorldClearInactiveScene(void) {
    RenderWorld *world = GameRenderWorldMutable();

    world->hasCamera = 0;
    world->hasMirrorCamera = 0;
    world->mirrorActive = 0;
    world->instanceCount = 0;
    world->overflowCount = 0;
}

/*
 * The environment palette as the frame being marked sees it. These slots
 * change with the course and with the time of day, so reading them from a
 * separate run and comparing them with a captured picture compares two
 * different moments; every sky colour question this port has had was made
 * harder by that. Write them beside the picture instead.
 */
void GameRenderWorldEnvironmentPalette(unsigned char out[9][3]) {
    int slot;
    for (slot = 0; slot < ENV_SLOT_COUNT; slot++) {
        out[slot][0] = g_EnvironmentColors.fields.slots[slot].cur.bytes.r;
        out[slot][1] = g_EnvironmentColors.fields.slots[slot].cur.bytes.g;
        out[slot][2] = g_EnvironmentColors.fields.slots[slot].cur.bytes.b;
    }
}

static uint32_t TrackDataAssetKey(void) {
    uint32_t current = (uint32_t)(ASSET_TRACK_2ND_BASE +
                                  g_GrandPrixClass * 8 + g_CourseIndex * 2);
    return TrackAssetIdentityResolve(current);
}

static uint32_t CarEntity(const GameCarRuntime *object) {
    uintptr_t address = (uintptr_t)object;
    uintptr_t first = (uintptr_t)&g_Cars[0];
    uintptr_t pastLast = (uintptr_t)&g_Cars[RACE_CAR_SLOT_COUNT];

    if (address >= first && address < pastLast &&
        (address - first) % sizeof(g_Cars[0]) == 0) {
        return (uint32_t)((address - first) / sizeof(g_Cars[0]));
    }
    return RAGE_PLAYER_CAR_ENTITY;
}

static uint8_t TrackCarMaterialVariant(uint8_t paletteOffset) {
    int variant = s_trackCarAsset;
    if ((variant < 0 || variant >= 32) && g_CarTable != NULL &&
        g_PlayerCarIndex >= 0 && g_PlayerCarIndex < 32) {
        variant = GetCarAssetIndex(
            g_PlayerCarIndex, g_CarTable[g_PlayerCarIndex].modelVariant);
    }
    if (variant < 0 || variant >= 32) variant = 0;
    if (paletteOffset > 2) paletteOffset = 0;
    return (uint8_t)(variant * 3 + paletteOffset);
}

void GameRenderWorldSetTrackCarAsset(int asset) {
    s_trackCarAsset = asset >= 0 && asset < 32 ? asset : -1;
}

/* Set while the in-car view publishes the player's body for its lamps only:
 * neither the rasterizer nor the ray scene ever sees it. */
static int s_playerCarLampsOnly;

static void GameRenderWorldSubmitCarPart(uint32_t entity, uint32_t part,
                                             uint32_t asset,
                                             RenderAssetSet assetSet,
                                             uint32_t mesh,
                                             RenderAssetSource source,
                                             uint8_t materialVariant,
                                             Vec3 psPosition,
                                             SceneMat3 rotation,
                                             Vec3 environmentLight,
                                             int mirror_pass,
                                             const CarEntry *paint) {
    RenderMeshInstance instance;
    if (part >= RAGE_CAR_RENDER_PART_COUNT) return;
    memset(&instance, 0, sizeof(instance));
    instance.entity = entity;
    instance.component = (uint8_t)part;
    instance.mesh = mesh;
    instance.assetSet = assetSet;
    instance.assetKey = asset;
    instance.assetSource = source;
    if (assetSet == RAGE_RENDER_ASSET_MODEL_BANK && paint != NULL &&
        paint->paintColor1 < RAGE_CAR_PAINT_COLOR_COUNT &&
        paint->paintColor2 < RAGE_CAR_PAINT_COLOR_COUNT) {
        instance.hasCarPaint = 1;
        instance.carPaintColor1 = paint->paintColor1;
        instance.carPaintColor2 = paint->paintColor2;
    }
    instance.materialVariant = materialVariant;
    instance.pass = mirror_pass ? RAGE_RENDER_PASS_MIRROR : RAGE_RENDER_PASS_MAIN;
    /* Cars are depth-cued like every other polygon on the PS1. */
    instance.flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING |
                     RAGE_RENDER_INSTANCE_ENABLE_FOG;
    if (entity == RAGE_PLAYER_CAR_ENTITY && part == 0 && !mirror_pass)
        instance.flags |= RAGE_RENDER_INSTANCE_FOCUS;
    if (s_playerCarLampsOnly)
        instance.flags |= RAGE_RENDER_INSTANCE_LAMPS_ONLY;
    instance.environmentLight = environmentLight;
    const CarPart currentPart = {psPosition, rotation};
    instance.transform = CarPartTransform(&currentPart);
    if (s_havePreviousCars[entity][part])
        instance.previousTransform = s_previousCars[entity][part];
    else {
        instance.previousTransform = instance.transform;
        s_havePreviousCars[entity][part] = 1;
    }
    s_previousCars[entity][part] = instance.transform;
    RenderWorldSubmitMesh(GameRenderWorldMutable(), &instance);
}

void GameRenderWorldBeginFrame(uint64_t frame) {
    s_haveSkyGrid[0] = 0;
    s_haveSkyGrid[1] = 0;
    if (!s_initialized) {
        for (int i = 0; i < 3; ++i)
            RenderWorldInit(&s_worlds[i], s_instances[i],
                RAGE_GAME_RENDER_WORLD_MAX_INSTANCES);
        s_initialized = 1;
    }
    if (s_verifyPublication < 0)
        s_verifyPublication = getenv("RAGE_VERIFY_WORLD_PUBLICATION") != NULL;
    if (s_verifyPublication) {
        s_publishedHash = WorldPublicationHash(&s_worlds[s_publishedWorld]);
        s_previousHash = WorldPublicationHash(&s_worlds[s_previousWorld]);
    }
    if (s_publishedCount) {
        /* Copy completed metadata, not the contents from a reused older slot.
         * Instance storage remains private to the building world. */
        *GameRenderWorldMutable() = s_worlds[s_publishedWorld];
        GameRenderWorldMutable()->instances = s_instances[s_buildingIndex];
    }
    memset(s_carLightInputs, 0, sizeof(s_carLightInputs));
    RenderWorldBeginFrame(GameRenderWorldMutable(), frame);
    if (!GameSceneUsesRaceWorld()) {
        GameRenderWorldClearInactiveScene();
    }
    s_buildingWorld = 1;
}

static void PublishCarLights(void) {
    RenderWorld *world = GameRenderWorldMutable();
    const RenderWorld *previous = GameRenderWorldCurrent();
    if (!GameSceneUsesRaceWorld()) return;
    float daylight = CarLightDaylight(world->camera.skyTopColor,
                                      world->camera.skyHorizonColor);
    float seconds = g_RacePaused ? 0.0f : 1.0f / (float)TimingBaseHz();
    for (uint32_t i = 0; i < world->instanceCount; ++i) {
        RenderMeshInstance *body = &world->instances[i];
        if (body->component != 0 || body->entity >= RAGE_CAR_ENTITY_COUNT ||
            (body->assetSet != RAGE_RENDER_ASSET_MODEL_BANK &&
             body->assetSet != RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1)) continue;
        if (body->assetSource == RENDER_ASSET_OWNED) continue;
        if (!s_carLightInputs[body->entity].present) continue;
        if (previous) {
            for (uint32_t j = 0; j < previous->instanceCount; ++j) {
                const RenderMeshInstance *old = &previous->instances[j];
                if (old->entity == body->entity && old->component == 0 &&
                    old->assetKey == body->assetKey &&
                    old->assetSet == body->assetSet && old->pass == body->pass) {
                    body->lamps = old->lamps;
                    break;
                }
            }
        }
        UpdateCarLights(&body->lamps, daylight,
                       s_carLightInputs[body->entity].zoneDaylight,
                       s_carLightInputs[body->entity].braking, seconds);
    }
}

void GameRenderWorldEndFrame(void) {
    if (!s_initialized || !s_buildingWorld) return;
    PublishCarLights();
    if (s_verifyPublication) {
        if (s_publishedHash != WorldPublicationHash(&s_worlds[s_publishedWorld]) ||
            s_previousHash != WorldPublicationHash(&s_worlds[s_previousWorld])) {
            fprintf(stderr, "world-publication verify=MISMATCH\n");
            abort();
        }
        if (g_SceneId == 12 && GameRenderWorldMutable()->frame % 128 == 0)
            fprintf(stderr, "world-publication verify=match frame=%llu\n",
                (unsigned long long)GameRenderWorldMutable()->frame);
    }
    int retired = s_previousWorld;
    s_previousWorld = s_publishedWorld;
    s_publishedWorld = s_buildingIndex;
    s_buildingIndex = retired;
    if (s_publishedCount < 2) ++s_publishedCount;
    s_buildingWorld = 0;
}

static RenderCamera GameRenderWorldBuildCamera(
    int32_t x, int32_t y, int32_t z, int32_t pitch, int32_t yaw, int32_t roll,
    float verticalFovDegrees, int rearFacing) {
    RenderCamera camera;
    SceneMat3 view;
    GameSkyGridLayout skyGrid;

    memset(&camera, 0, sizeof(camera));
    camera.transform.position.x = (float)x;
    camera.transform.position.y = -(float)y;
    camera.transform.position.z = -(float)z;
    /* SetCameraRotMatrix publishes a view matrix: Rz(roll)*Rx(pitch)*Ry(yaw).
     * Scene data needs the inverse as a camera pose. Converting the actual
     * matrix is unambiguous and avoids angle-sign heuristics around 180°. */
    view = SceneMat3Multiply(
        SceneMat3Multiply(SceneRotationZ(roll), SceneRotationX(pitch)),
        SceneRotationY(yaw));
    if (rearFacing) {
        /* A mirror camera turns in its own local space. Adding 180 degrees
         * to world yaw gives the wrong direction once the car is pitched or
         * rolled; pre-rotate the view basis like an attached camera rig. */
        view = SceneMat3Multiply(SceneRotationY(0x800), view);
    }
    {
        SceneMat3 converted;
        RenderConvertPsxMatrix(view.m, converted.m);
        camera.transform.orientation = SceneQuaternion(
            SceneMat3Transpose(converted));
    }
    camera.transform.hasOrientation = 1;
    camera.transform.rotation.x = -AngleToDegrees(pitch);
    camera.transform.rotation.y = -AngleToDegrees(yaw);
    camera.transform.rotation.z = -AngleToDegrees(roll);
    camera.transform.scale.x = 1.0f;
    camera.transform.scale.y = 1.0f;
    camera.transform.scale.z = 1.0f;
    camera.verticalFovDegrees = verticalFovDegrees;
    camera.nearPlane = 1.0f;
    /* Race geometry uses the verified scene-depth limit together with the
     * authored region masks. Non-race presentation retains its stage range. */
    camera.farPlane = (float)RuntimeConfigInt(
        "diagnostics.native_far_plane",
        GameSceneUsesRaceWorld() ? 16384 : 262144, 1024, 262144);
    /* Convert the authored environment palette into semantic sky bands. The
     * native backend owns their projection; it never replays DrawSkyBackground
     * packets or depends on an ordering-table bucket. */
    /*
     * The gradient runs slot 1 overhead, through 2, to slot 3 at the skyline,
     * and slot 4 is the dark band below it. Read off a marker whose palette
     * was saved with it: slot 2 is 0,72,136 and slot 3 is 96,136,184, and
     * the classic renderer's own pixels walk between exactly those two
     * across the visible sky.
     */
    camera.skyAssetKey = TrackDataAssetKey();
    const Environment environment = {.colors = g_EnvironmentColors,
        .fogNear = g_FogNear, .skyRowBase = g_SkyRowBase};
    ApplyEnvironment(&camera, &environment);
    RageSkyCapturePanoramaLayout(&camera.skyLayout, g_SkyTileMap, g_SkyRowBase);
    camera.hasSkyLayout = 1;
    if (s_haveSkyGrid[rearFacing != 0]) {
        skyGrid = s_skyGrid[rearFacing != 0];
    } else {
        MeasureSkyGridLayout(y, pitch, yaw, roll, g_RenderState.pass.orderingFlag,
                             g_MirrorMode, &skyGrid);
    }
    camera.skyGridOrigin.x = skyGrid.panelXFixed * (1.0f / 256.0f);
    camera.skyGridOrigin.y = skyGrid.panelYFixed * (1.0f / 256.0f);
    camera.skyGridOrigin.z = skyGrid.lowerPanelXFixed * (1.0f / 256.0f);
    camera.skyGridColumn.x = skyGrid.columnStepX * (1.0f / 256.0f);
    camera.skyGridColumn.y = skyGrid.columnStepY * (1.0f / 256.0f);
    camera.skyGridColumn.z = (float)skyGrid.textureColumn;
    camera.skyGridRow.x = skyGrid.rowStepX * (1.0f / 256.0f);
    camera.skyGridRow.y = skyGrid.rowStepY * (1.0f / 256.0f);
    camera.skyGridRow.z = skyGrid.lowerPanelYFixed * (1.0f / 256.0f);
    /* Course geometry is stored in GTE units while Render World uses the
     * game's world units (four GTE units each). SetFogNear reaches full fog
     * at five times its authored near distance. */
    return camera;
}

void GameRenderWorldSetSkyGrid(const GameSkyGridLayout *layout,
                               int mirrorPass) {
    int index = mirrorPass != 0;
    if (layout == NULL) return;
    s_skyGrid[index] = *layout;
    s_haveSkyGrid[index] = 1;
}

void GameRenderWorldBeginSkyPackets(void) { CaptureSkyBegin(); }

void GameRenderWorldEndSkyPackets(void) { CaptureSkyEnd(); }

void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z,
                                  int32_t pitch, int32_t yaw, int32_t roll) {
    RenderCamera camera;
    RenderDirectionalLight light;

    if (!s_initialized) return;
    /* PAL's 320x240 active viewport with geom screen 320: 41.112°. */
    camera = GameRenderWorldBuildCamera(x, y, z, pitch, yaw, roll,
                                            41.112f, 0);
    RenderDirectionalLightFromSky(&camera, &light);
    RenderWorldSetDirectionalLight(GameRenderWorldMutable(), &light);
    RenderWorldSetCamera(GameRenderWorldMutable(), &camera);
}

void GameRenderWorldPublishCurrentCamera(void) {
    RenderCamera mirrorCamera;
    int mirrorActive;

    /* A scene transition can happen during dispatch, after BeginFrame saw the
     * race scene. Clear the publication again here so the transition frame
     * cannot present its stale 3D world. */
    if (!GameSceneUsesRaceWorld()) {
        GameRenderWorldClearInactiveScene();
        return;
    }
    GameRenderWorldSetCamera(g_Camera.view.x, g_Camera.view.y,
                                 g_Camera.view.z, g_Camera.view.angleX,
                                 g_Camera.view.angleY,
                                 g_Camera.view.angleZ);
    /* A car mirror is a second scene camera, not a recreation of the PS1
     * mirror pass. A 20 degree vertical FOV on the wide mirror target gives
     * a useful rearward field of view without the old projection distortion. */
    mirrorCamera = GameRenderWorldBuildCamera(
        g_Camera.view.x, g_Camera.view.y, g_Camera.view.z,
        g_Camera.view.angleX, g_Camera.view.angleY,
        g_Camera.view.angleZ, 20.0f, 1);
    /* The tiny, wide mirror loses useful silhouettes when it reaches the
     * main view's full fog distance. It already consumes the complete native
     * scene, so extend only its semantic fog range rather than reviving the
     * PS1 mirror visibility list. */
    mirrorCamera.fogNear *= 2.0f;
    mirrorCamera.fogFar *= 2.0f;
    mirrorActive = g_MirrorUnlocked != 0 && g_RenderState.mirror.enabled != 0 &&
                   g_Camera.mode == CAMERA_VIEW_CAR &&
                   g_GrandPrixMode != 0 &&
                   g_RacePhase == RACE_PHASE_ACTIVE;
    RenderWorldSetMirrorCamera(GameRenderWorldMutable(), &mirrorCamera,
                                   mirrorActive, (float)g_MirrorPanelY);
}

static void GameRenderWorldSubmitCourseTransform(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    SceneMat3 rotation, int fogged, int mirror_pass,
    int cullBackfaces, int depthOverlay, float depthBias,
    uint8_t paletteOffset, int rayOnly) {
    RenderMeshInstance instance;

    if (!s_initialized || mesh < 0) return;
    memset(&instance, 0, sizeof(instance));
    instance.entity = entity;
    instance.mesh = (uint32_t)mesh;
    instance.assetSet = RAGE_RENDER_ASSET_COURSE;
    instance.assetKey = TrackDataAssetKey();
    instance.material = 0;
    instance.materialVariant =
        (uint8_t)(((g_TrackTexturePageWanted != 0) ? 4u : 0u) +
                  (paletteOffset & 3u));
    instance.textureScrollU = (uint8_t)(g_AnimTimer & 0x7F);
    instance.depthBias = depthBias;
    instance.pass = mirror_pass ? RAGE_RENDER_PASS_MIRROR : RAGE_RENDER_PASS_MAIN;
    instance.transform.position.x =
        (float)CourseCoordinateNearReference(x, g_Camera.view.x);
    instance.transform.position.y =
        -(float)CourseCoordinateNearReference(y, g_Camera.view.y);
    instance.transform.position.z =
        -(float)CourseCoordinateNearReference(z, g_Camera.view.z);
    instance.transform.orientation = SceneQuaternionFromPsx(rotation);
    instance.transform.hasOrientation = 1;
    instance.transform.scale.x = 0.25f;
    instance.transform.scale.y = 0.25f;
    instance.transform.scale.z = 0.25f;
    instance.flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL |
                     RAGE_RENDER_INSTANCE_FLAT_SHADED;
    /* Animated screen art is emissive/unlit. Its structural backing and
     * ordinary course scenery participate in native scene lighting. */
    if (!depthOverlay)
        instance.flags |= RAGE_RENDER_INSTANCE_ENABLE_LIGHTING;
    instance.lightInfluence = depthOverlay ? 0.0f : 0.65f;
    if (fogged) instance.flags |= RAGE_RENDER_INSTANCE_ENABLE_FOG;
    if (cullBackfaces)
        instance.flags |= RAGE_RENDER_INSTANCE_CULL_BACKFACES;
    if (depthOverlay)
        instance.flags |= RAGE_RENDER_INSTANCE_DEPTH_DECAL;
    if (rayOnly)
        instance.flags |= RAGE_RENDER_INSTANCE_RAY_ONLY;
    instance.previousTransform = instance.transform;
    RenderWorldSubmitMesh(GameRenderWorldMutable(), &instance);
}

void GameRenderWorldSubmitCourseObject(uint32_t entity, int32_t mesh,
                                           int32_t x, int32_t y, int32_t z,
                                           int32_t yaw, int fogged,
                                           int mirror_pass) {
    GameRenderWorldSubmitCourseTransform(
        0x10000u + entity, mesh, x, y, z, SceneRotationY(yaw), fogged,
        mirror_pass, 1, 0, 0.0f, 0, 0);
}

static void GameRenderWorldSubmitDynamicCourseObjectInternal(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass,
    int cullBackfaces, int depthOverlay, float depthBias) {
    SceneMat3 matrix;
    RenderWorld *world;
    uint32_t semanticEntity = 0x30000u + entity;
    int row, column;
    if (rotation == NULL) return;
    world = GameRenderWorldMutable();
    /* Legacy draws visit dynamic scenery once per camera. Render World owns
     * scene objects rather than camera submissions, so retain the first
     * world-space record and do not draw two nearly identical copies in the
     * native main and rear-camera passes. Main is submitted before mirror;
     * a mirror-only object is still retained when it is behind the car. */
    for (uint32_t index = 0; index < world->instanceCount; index++) {
        const RenderMeshInstance *existing = &world->instances[index];
        if (existing->entity == semanticEntity &&
            existing->assetSet == RAGE_RENDER_ASSET_COURSE) return;
    }
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++)
            matrix.m[row][column] =
                (float)rotation[row][column] * (1.0f / 4096.0f);
    /* Animated screen layers are authored as flat quads and are visible from
     * both replay directions. The native GPU already handles their support
     * surface with a depth buffer; applying the course-object winding test
     * removes the entire image while leaving the black screen frame. */
    GameRenderWorldSubmitCourseTransform(
        semanticEntity, mesh, x, y, z, matrix, fogged, mirror_pass,
        cullBackfaces,
        depthOverlay, depthBias,
        (uint8_t)((g_RenderState.geometry.envMode4 >> 16) & 3), 0);
}

void GameRenderWorldSubmitDynamicCourseObject(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass) {
    GameRenderWorldSubmitDynamicCourseObjectInternal(
        entity, mesh, x, y, z, rotation, fogged, mirror_pass, 1, 0, 0.0f);
}

void GameRenderWorldSubmitStartGridScenery(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass) {
    GameRenderWorldSubmitDynamicCourseObjectInternal(
        entity, mesh, x, y, z, rotation, fogged, mirror_pass, 1, 0,
        START_GRID_DEPTH_BIAS);
}

void GameRenderWorldSubmitDynamicCourseOverlay(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass) {
    GameRenderWorldSubmitDynamicCourseObjectInternal(
        entity, mesh, x, y, z, rotation, fogged, mirror_pass, 0, 1, 0.0f);
}

static void SubmitTerrainCell(uint32_t grid_x, uint32_t grid_z,
                              int32_t mesh, int mirror_pass, int rayOnly) {
    RenderMeshInstance instance;

    if (!s_initialized || mesh < 0) return;
    memset(&instance, 0, sizeof(instance));
    instance.entity = 0x20000u + grid_z * 32u + grid_x;
    instance.mesh = (uint32_t)mesh;
    instance.assetSet = RAGE_RENDER_ASSET_TERRAIN;
    instance.assetKey = TrackDataAssetKey();
    instance.material = 0;
    instance.materialVariant =
        (uint8_t)(((g_TrackTexturePageWanted != 0) ? 2u : 0u) +
                  (g_IsEnvironmentMode4 ? 1u : 0u));
    instance.pass = mirror_pass ? RAGE_RENDER_PASS_MIRROR : RAGE_RENDER_PASS_MAIN;
    /* Terrain vertices and their 8192-unit cell pitch are GTE units (four
     * per game-world unit). `grid_z` is the game's sy, even though the source
     * grid stores it in row 31-sy. Convert it once into the common scene
     * coordinate system, then let the normal mesh-bound frustum culler select
     * visible cells instead of submitting all 1024 authored cells. */
    instance.transform.position.x = (float)(grid_x * 2048u + 1024u);
    instance.transform.position.z = -(float)(grid_z * 2048u + 1024u);
    instance.transform.scale.x = 0.25f;
    instance.transform.scale.y = 0.25f;
    instance.transform.scale.z = 0.25f;
    /* The PS1 depth-cues every course polygon toward the environment's far
     * colour, road and surroundings included. Without the flag the native
     * terrain never fogged and only a few scenery models did. */
    instance.flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL |
                     RAGE_RENDER_INSTANCE_ENABLE_LIGHTING |
                     RAGE_RENDER_INSTANCE_FLAT_SHADED |
                     RAGE_RENDER_INSTANCE_ENABLE_FOG;
    instance.lightInfluence = 0.65f;
    if (g_IsEnvironmentMode4)
        instance.flags |= RAGE_RENDER_INSTANCE_ENVIRONMENT_MODE_4;
    if (rayOnly)
        instance.flags |= RAGE_RENDER_INSTANCE_RAY_ONLY;
    instance.previousTransform = instance.transform;
    RenderWorldSubmitMesh(GameRenderWorldMutable(), &instance);
}

void GameRenderWorldSubmitTerrainCell(uint32_t grid_x, uint32_t grid_z,
                                      int32_t mesh, int mirror_pass) {
    SubmitTerrainCell(grid_x, grid_z, mesh, mirror_pass, 0);
}

/* The original per-region masks change with camera position and retain the
 * long straight without exposing unrelated sections above nearby scenery.
 * Main and mirror share a camera region; heading does not affect these masks. */
static int NativeRegionAllowsCell(int32_t cellX, int32_t cellZ) {
    _Static_assert(TERRAIN_CELL_GRID_SIZE == 32 &&
                   TERRAIN_CELL_REGION_SHIFT == 10,
                   "native visibility must match the original table layout");
    static int enabled = -1;
    const RenderWorld *world = GameRenderWorldMutable();
    if (enabled < 0)
        enabled = RuntimeConfigGet("diagnostics.native_region_visibility") == NULL ||
                  RuntimeConfigEnabled("diagnostics.native_region_visibility");
    if (!enabled || !GameSceneUsesRaceWorld() || !world->hasCamera) return 1;
    return NativeVisibilityAllowsCell(g_TerrainCellGrid, g_CellVisibilityTable,
        world->camera.transform.position.x, -world->camera.transform.position.z,
        cellX, cellZ);
}

void GameRenderWorldPublishTerrainGrid(void) {
    uint32_t grid_z;
    static int trace = -1;
    static uint32_t previousCells[32];
    uint32_t cells[32] = {0};

    if (!s_initialized || g_TerrainCellGrid == NULL) return;
    if (trace < 0)
        trace = RuntimeConfigEnabled("diagnostics.native_visibility_trace");
    for (grid_z = 0; grid_z < 32; grid_z++) {
        uint32_t grid_x;
        for (grid_x = 0; grid_x < 32; grid_x++) {
            int32_t mesh = g_TerrainCellGrid[((31u - grid_z) << 5) + grid_x]
                         & 0x3FF;
            int allowed = (trace || mesh != 0x3FF) &&
                          NativeRegionAllowsCell(grid_x, grid_z);
            if (trace && allowed) cells[grid_z] |= UINT32_C(1) << grid_x;
            if (mesh != 0x3FF)
                SubmitTerrainCell(grid_x, grid_z, mesh, 0, !allowed);
        }
    }
    if (trace && memcmp(cells, previousCells, sizeof(cells))) {
        const RenderWorld *world = GameRenderWorldMutable();
        fprintf(stderr, "rage-port: visibility-change frame=%llu course=%d "
                "camera=%.0f,%.0f far=%.0f\n",
                (unsigned long long)world->frame, g_CourseIndex,
                world->camera.transform.position.x,
                -world->camera.transform.position.z, world->camera.farPlane);
        for (grid_z = 0; grid_z < 32; ++grid_z) {
            uint32_t added = cells[grid_z] & ~previousCells[grid_z];
            uint32_t removed = previousCells[grid_z] & ~cells[grid_z];
            if (added || removed)
                fprintf(stderr, "rage-port: visibility-row z=%u added=%08x removed=%08x\n",
                        grid_z, added, removed);
        }
        memcpy(previousCells, cells, sizeof(cells));
    }
}

void GameRenderWorldPublishCourseObjects(void) {
    int32_t i;
    if (!s_initialized || g_CourseObjects == NULL) return;
    for (i = 0; i < g_CourseObjectCount; i++) {
        const CourseObject *object = &g_CourseObjects[i];
        if (object->modelId < 0) continue;
        int allowed = NativeRegionAllowsCell(
            object->x / 2048, object->z / 2048);
        /* This is intentionally not gated by the 32x32 classic scan list:
         * it exists for the classic OT/GTE emitter.  The native path
         * keeps semantic scene data complete and applies normal frustum/depth
         * visibility when it builds GPU draws. */
        GameRenderWorldSubmitCourseTransform(
            0x10000u + (uint32_t)i, object->modelId,
            object->x, object->y, object->z,
            SceneRotationY(object->rotationY),
            g_IsEnvironmentMode4
                ? (object->flags &
                   COURSE_OBJECT_ALTERNATE_ENVIRONMENT_4) != 0
                : (object->flags & COURSE_OBJECT_ALTERNATE_NORMAL) != 0,
            0, 1, 0, 0.0f, 0, !allowed);
    }
}

static void GameRenderWorldSubmitCarAssembly(const GameCarRuntime *object,
                                                 uint32_t entity, uint32_t asset,
                                                 RenderAssetSet assetSet,
                                                 uint32_t bodyMesh,
                                                 uint32_t frontWheelMesh,
                                                 uint32_t rearWheelMesh,
                                                 RenderAssetSource source,
                                                 uint8_t bodyMaterialVariant,
                                                 s16 horizon, s16 offsetX,
                                                 s16 offsetY, s16 offsetZ,
                                                 s32 steeringAngle,
                                                 Vec3 environmentLight,
                                                 int mirror_pass,
                                                 const CarEntry *paint) {
    const CarShape shape = {offsetX, offsetY, offsetZ, horizon};
    CarPart parts[CAR_PART_COUNT];
    if (!BuildCarParts(object, &shape, steeringAngle, parts)) return;
    const uint32_t components[CAR_PART_COUNT] = {0, 2, 3, 4};
    const uint32_t meshes[CAR_PART_COUNT] = {
        bodyMesh, rearWheelMesh, frontWheelMesh, frontWheelMesh
    };
    const uint8_t wheelMaterialVariant = assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1
        ? (uint8_t)(bodyMaterialVariant / 3u * 3u) : 0;
    /* The retail shadow plate is omitted; shadows use actual car geometry. */
    for (unsigned i = 0; i < CAR_PART_COUNT; i++) {
        GameRenderWorldSubmitCarPart(entity, components[i], asset, assetSet,
            meshes[i], source, i == 0 ? bodyMaterialVariant : wheelMaterialVariant, parts[i].position,
            parts[i].rotation, environmentLight, mirror_pass, paint);
    }
}

static Vec3 GameTrackLightForCar(const GameCarRuntime *object, const TrackZoneEffect *effect) {
    Vec3 result = {1.0f, 1.0f, 1.0f};
    float light[3];
    TrackZoneEffect zone;
    /* Live race and attract playback share one native scene treatment.
     * Scripted presentation scenes keep their authored neutral appearance. */
    if (!effect && !GameSceneUsesRaceWorld()) return result;
    zone = effect ? *effect : GetTrackZoneEffect(object->trackProgress);
    TrackZoneLightColor(zone.blend, zone.code, light);
    result.x = light[0];
    result.y = light[1];
    result.z = light[2];
    return result;
}

void GameRenderWorldSubmitCar(const GameCarRuntime *object, int mirror_pass,
                            RageGameCarRenderDetail detail) {
    GameRenderWorldSubmitRivalCar(object, CarEntity(object), mirror_pass, detail);
}

void GameRenderWorldSubmitRivalCar(const GameCarRuntime *object,
                                  uint32_t entity, int mirror_pass,
                                  RageGameCarRenderDetail detail) {
    int car;
    const s16 *lod;
    Vec3 environmentLight;

    if (!s_initialized || object == NULL || g_TrackRenderTable == NULL ||
        entity >= RAGE_CAR_ENTITY_COUNT ||
        (u32)object->modelIndex >= RACE_CAR_SLOT_COUNT) return;
    /* Rival and traffic cars belong to the race world. The custom race
     * showroom draws its rival preview through the same path from a private
     * bank; publishing that would import the wrong bank under the track's
     * asset key and keep it for the race. */
    if (!GameSceneUsesRaceWorld()) return;

    CaptureCarLightInput(entity, object, NULL);
    environmentLight = GameTrackLightForCar(object, NULL);
    car = g_CarModelByCourse[SeriesCourseIndex()][object->modelIndex];
    lod = g_CarModelBankTable[car];
    if (detail == RAGE_GAME_CAR_RENDER_FAR) {
        SceneMat3 body = SceneMat3Multiply(
            SceneMat3Multiply(
                SceneRotationY(0x800 - object->bodyYaw),
                SceneRotationX(object->bodyPitch)),
            SceneRotationZ(object->bodyRoll));
        Vec3 origin = {
            (float)object->x,
            (float)(object->y - g_TrackRenderTable->models[car].horizon),
            (float)object->z,
        };
        GameRenderWorldSubmitCarPart(
            entity, 0, TrackDataAssetKey(),
            RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
            (uint32_t)lod[0] + 4u, RENDER_ASSET_DEFAULT, TrackCarMaterialVariant((uint8_t)lod[1]), origin, body,
            environmentLight, mirror_pass, NULL);
        return;
    }
    GameRenderWorldSubmitCarAssembly(object, entity, TrackDataAssetKey(),
        RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
        (uint32_t)lod[0], (uint32_t)lod[0] + 2u, (uint32_t)lod[0] + 3u,
        RENDER_ASSET_DEFAULT, TrackCarMaterialVariant((uint8_t)lod[1]),
        g_TrackRenderTable->models[car].horizon,
        g_TrackRenderTable->models[car].axis0,
        (s16)g_TrackRenderTable->models[car].axis1,
        (s16)g_TrackRenderTable->models[car].axis2,
        object->steeringAngle * 2, environmentLight, mirror_pass, NULL);
}

void GameRenderWorldSubmitPlayerCarLamps(const GameCarRuntime *object) {
    s_playerCarLampsOnly = 1;
    const CarEntry *paint = g_CarTable != NULL &&
        (u32)g_PlayerCarIndex < CUSTOM_PAINT_CAR_COUNT
        ? &g_CarTable[g_PlayerCarIndex] : NULL;
    GameRenderWorldSubmitPlayerCar(
        object, RAGE_PLAYER_CAR_ENTITY, g_CarModelAsset, paint, 0);
    s_playerCarLampsOnly = 0;
}

void GameRenderWorldSubmitPlayerCar(const GameCarRuntime *object,
                                   uint32_t entity,
                                   const CarModelAsset *modelAsset,
                                   const CarEntry *paint, int mirror_pass) {
    if (modelAsset == NULL) return;
    const s32 slot = FindCarModelSlot(modelAsset);
    s32 variant = slot >= 0 ? g_CarModelSlotAssetIndex[slot] : -1;
    if (variant < 0) {
        if (g_CarTable == NULL || (u32)g_PlayerCarIndex >= GAME_CAR_COUNT) return;
        variant = GetCarAssetIndex(
            g_PlayerCarIndex, g_CarTable[g_PlayerCarIndex].modelVariant);
    }
    const CarShape shape = {modelAsset->modelOffsetX, modelAsset->modelOffsetY,
                            modelAsset->modelOffsetZ, modelAsset->horizon};
    GameRenderWorldSubmitHumanCar(object, entity, variant, &shape, paint,
                                 mirror_pass);
}

void GameRenderWorldSubmitHumanCar(const GameCarRuntime *object,
                                  uint32_t entity, s32 variant,
                                  const CarShape *shape,
                                  const CarEntry *paint, int mirror_pass) {
    if (!s_initialized || object == NULL || shape == NULL ||
        (u32)variant >= CAR_MODEL_VARIANT_COUNT ||
        entity >= RAGE_CAR_ENTITY_COUNT) return;
    uint32_t asset = (uint32_t)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variant);
    CaptureCarLightInput(entity, object, NULL);
    Vec3 environmentLight = GameTrackLightForCar(object, NULL);
    uint32_t wheelBase = (uint32_t)object->renderDepth * 2u;
    if ((object->wheelRotation & 0x1000) != 0) wheelBase += 10u;
    if (wheelBase + 3u >= 22u) wheelBase = 0;
    /* Native model materials own their CLUTs; only rival banks use lod[1]. */
    GameRenderWorldSubmitCarAssembly(object, entity, asset,
        RAGE_RENDER_ASSET_MODEL_BANK, 0, wheelBase + 2u, wheelBase + 3u,
        GameRenderWorldMutable()->explicitCars ? RENDER_ASSET_OWNED : RENDER_ASSET_DEFAULT, 0,
        shape->horizon, shape->offsetX, shape->offsetY, shape->offsetZ,
        object->steeringAngle / 12, environmentLight, mirror_pass, paint);
}

void GameRenderWorldPublishRaceCars(void) {
    RenderWorld *world;
    uint32_t source, destination = 0;
    int car;

    if (!s_initialized || !GameSceneUsesRaceWorld() ||
        (g_SceneId == 12 && g_GrandPrixMode == 0)) return;
    world = GameRenderWorldMutable();
    if (world->explicitCars) return;
    /* DrawCar historically publishes only rivals accepted by the active GTE
     * view. Replace those partial main-camera submissions with one complete
     * semantic traffic list. Keep the separately loaded player model and
     * deprecated mirror-pass records untouched. */
    for (source = 0; source < world->instanceCount; source++) {
        const RenderMeshInstance *instance = &world->instances[source];
        if (instance->pass == RAGE_RENDER_PASS_MAIN &&
            instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) {
            continue;
        }
        if (destination != source)
            world->instances[destination] = world->instances[source];
        destination++;
    }
    world->instanceCount = destination;
    for (car = 0; car < RACE_CAR_SLOT_COUNT; car++) {
        if (g_Cars[car].activeFlag != -1 && g_Cars[car].aiEnabled == 1) {
            GameRenderWorldSubmitCar(
                &g_Cars[car], 0,
                RAGE_GAME_CAR_RENDER_CLOSE);
        }
    }
}

void GameRenderWorldDiscardLegacyMirror(void) {
    if (!s_initialized) return;
    /* The native rear-view camera renders the ordinary semantic main scene.
     * PS1 mirror submissions are camera-space implementation records and
     * must never survive into that scene. The legacy renderer has already
     * consumed them through its own capture path. */
    RenderWorldDiscardPass(GameRenderWorldMutable(),
                               RAGE_RENDER_PASS_MIRROR);
}

const RenderWorld *GameRenderWorldCurrent(void) {
    return s_publishedCount ? &s_worlds[s_publishedWorld] : NULL;
}

const RenderWorld *GameRenderWorldPrevious(void) {
    return s_publishedCount >= 2 ? &s_worlds[s_previousWorld] : NULL;
}

const RenderWorld *GameRenderWorldPresentation(float t) {
    RenderWorld *current;
    const RenderWorld *previous;
    if (!s_publishedCount) return NULL;
    current = &s_worlds[s_publishedWorld];
    if (s_publishedCount < 2) return current;
    previous = &s_worlds[s_previousWorld];
    s_presentationWorld = *current;
    s_presentationWorld.instances = s_presentationInstances;
    s_presentationWorld.instanceCapacity =
        RAGE_GAME_RENDER_WORLD_MAX_INSTANCES;
    s_presentationWorld.instanceCount = 0;
    if (!RenderWorldTryBuildSynchronizedPresentation(
        previous, current, t, s_presentationInstances,
        RAGE_GAME_RENDER_WORLD_MAX_INSTANCES, &s_presentationWorld.instanceCount)) {
        /* A rejected presentation is not a valid empty scene. Keep the
         * incomplete-world signal explicit for backend completeness checks. */
        s_presentationWorld.overflowCount = 1;
    }
    RenderInterpolateCamera(&current->previousCamera, &current->camera, t,
                                &s_presentationWorld.camera);
    if (current->hasMirrorCamera) {
        RenderInterpolateCamera(&current->previousMirrorCamera,
                                    &current->mirrorCamera, t,
                                    &s_presentationWorld.mirrorCamera);
        s_presentationWorld.mirrorPanelY =
            current->previousMirrorPanelY +
            (current->mirrorPanelY - current->previousMirrorPanelY) * t;
    }
    /* Native preparation caches by frame id. Presentation may change several
     * times inside one logic tick, so it needs a separate revision. */
    s_presentationWorld.frame = ++s_presentationSerial;
    return &s_presentationWorld;
}
