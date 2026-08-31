#include "rage/render_world_game.h"

#include "course_coordinate.h"

#include <stddef.h>
#include <math.h>
#include <string.h>

#include "game/asset.h"
#include "game/player_car_internal.h"
#include "game/render.h"
#include "game/race.h"
#include "game/state.h"
#include "game/render_internal.h"
#include "game/track_internal.h"
#include "render/render_world_frame.h"
#include "render/car_paint.h"
#include "rage/track_asset_identity.h"
#include "rage/track_lighting.h"

enum { RAGE_GAME_RENDER_WORLD_MAX_INSTANCES = 4096 };

static RageRenderMeshInstance s_instances[2][RAGE_GAME_RENDER_WORLD_MAX_INSTANCES];
static RageRenderWorld s_worlds[2];
static RageRenderMeshInstance
    s_presentationInstances[RAGE_GAME_RENDER_WORLD_MAX_INSTANCES];
static RageRenderWorld s_presentationWorld;
static uint64_t s_presentationSerial;
enum { RAGE_CAR_RENDER_PART_COUNT = 6 };
static RageRenderTransform s_previousCars[12][RAGE_CAR_RENDER_PART_COUNT];
static uint8_t s_havePreviousCars[12][RAGE_CAR_RENDER_PART_COUNT];
static int s_trackCarAsset = -1;
static int s_initialized;
static int s_currentWorld;
static int s_haveCompletedFrame;

static int GameSceneUsesRaceWorld(void) {
    return g_SceneId == 12 || g_SceneId == 0x1E;
}

static RageRenderWorld *GameRenderWorldMutable(void) {
    return &s_worlds[s_currentWorld];
}

static float AngleToDegrees(s32 angle) {
    return (float)(angle & 0xFFF) * (360.0f / 4096.0f);
}

static void GameRenderWorldEnvironmentColor(int slot, RageRenderVec3 *out) {
    out->x =
        (float)g_EnvironmentColors.fields.slots[slot].cur.bytes.r / 255.0f;
    out->y =
        (float)g_EnvironmentColors.fields.slots[slot].cur.bytes.g / 255.0f;
    out->z =
        (float)g_EnvironmentColors.fields.slots[slot].cur.bytes.b / 255.0f;
}

static uint32_t TrackDataAssetKey(void) {
    uint32_t current = (uint32_t)(ASSET_TRACK_2ND_BASE +
                                  g_GrandPrixClass * 8 + g_CourseIndex * 2);
    return TrackAssetIdentityResolve(current);
}

static uint32_t CarEntity(const GameRenderObject *object) {
    const GameCarRuntime *car = (const GameCarRuntime *)object;
    ptrdiff_t index = car - g_Cars;
    if (index >= 0 && index < 11) return (uint32_t)index;
    return 11; /* Player storage is separate from g_Cars. */
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

typedef struct RageSceneMat3 {
    float m[3][3];
} RageSceneMat3;

static RageSceneMat3 SceneMat3Multiply(RageSceneMat3 a, RageSceneMat3 b) {
    RageSceneMat3 out = {{{0}}};
    int row, column, i;
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++)
            for (i = 0; i < 3; i++) out.m[row][column] += a.m[row][i] * b.m[i][column];
    return out;
}

static RageSceneMat3 SceneMat3Transpose(RageSceneMat3 source) {
    RageSceneMat3 out;
    int row, column;
    for (row = 0; row < 3; row++)
        for (column = 0; column < 3; column++) out.m[row][column] = source.m[column][row];
    return out;
}

static RageSceneMat3 SceneRotationX(s32 angle) {
    float a = AngleToDegrees(angle) * 0.017453292519943295f;
    float c = cosf(a), s = sinf(a);
    RageSceneMat3 out = {{{1, 0, 0}, {0, c, -s}, {0, s, c}}};
    return out;
}

static RageSceneMat3 SceneRotationY(s32 angle) {
    float a = AngleToDegrees(angle) * 0.017453292519943295f;
    float c = cosf(a), s = sinf(a);
    /* This is the game's BuildRotMatrixY convention, not a generic
     * right-handed Euler helper.  The PS1->scene basis conversion below
     * turns it into the renderer's conventional rotation. */
    RageSceneMat3 out = {{{c, 0, -s}, {0, 1, 0}, {s, 0, c}}};
    return out;
}

static RageSceneMat3 SceneRotationZ(s32 angle) {
    float a = AngleToDegrees(angle) * 0.017453292519943295f;
    float c = cosf(a), s = sinf(a);
    RageSceneMat3 out = {{{c, -s, 0}, {s, c, 0}, {0, 0, 1}}};
    return out;
}

static RageRenderQuaternion SceneQuaternion(RageSceneMat3 source) {
    RageRenderQuaternion out;
    float (*m)[3] = source.m;
    float trace, root;
    trace = m[0][0] + m[1][1] + m[2][2];
    if (trace > 0.0f) {
        root = sqrtf(trace + 1.0f) * 2.0f;
        out.w = 0.25f * root;
        out.x = (m[2][1] - m[1][2]) / root;
        out.y = (m[0][2] - m[2][0]) / root;
        out.z = (m[1][0] - m[0][1]) / root;
    } else if (m[0][0] > m[1][1] && m[0][0] > m[2][2]) {
        root = sqrtf(1.0f + m[0][0] - m[1][1] - m[2][2]) * 2.0f;
        out.w = (m[2][1] - m[1][2]) / root;
        out.x = 0.25f * root;
        out.y = (m[0][1] + m[1][0]) / root;
        out.z = (m[0][2] + m[2][0]) / root;
    } else if (m[1][1] > m[2][2]) {
        root = sqrtf(1.0f + m[1][1] - m[0][0] - m[2][2]) * 2.0f;
        out.w = (m[0][2] - m[2][0]) / root;
        out.x = (m[0][1] + m[1][0]) / root;
        out.y = 0.25f * root;
        out.z = (m[1][2] + m[2][1]) / root;
    } else {
        root = sqrtf(1.0f + m[2][2] - m[0][0] - m[1][1]) * 2.0f;
        out.w = (m[1][0] - m[0][1]) / root;
        out.x = (m[0][2] + m[2][0]) / root;
        out.y = (m[1][2] + m[2][1]) / root;
        out.z = 0.25f * root;
    }
    return out;
}

static RageRenderQuaternion SceneQuaternionFromPsx(RageSceneMat3 source) {
    RageSceneMat3 converted;
    RenderConvertPsxMatrix(source.m, converted.m);
    return SceneQuaternion(converted);
}

static RageRenderVec3 SceneRotatePoint(RageSceneMat3 matrix,
                                            float x, float y, float z) {
    RageRenderVec3 out;
    out.x = matrix.m[0][0] * x + matrix.m[0][1] * y + matrix.m[0][2] * z;
    out.y = matrix.m[1][0] * x + matrix.m[1][1] * y + matrix.m[1][2] * z;
    out.z = matrix.m[2][0] * x + matrix.m[2][1] * y + matrix.m[2][2] * z;
    return out;
}

static void GameRenderWorldSubmitCarPart(uint32_t entity, uint32_t part,
                                             uint32_t asset,
                                             RageRenderAssetSet assetSet,
                                             uint32_t mesh,
                                             uint8_t paletteOffset,
                                             RageRenderVec3 psPosition,
                                             RageSceneMat3 rotation,
                                             RageRenderVec3 environmentLight,
                                             int mirror_pass) {
    RageRenderMeshInstance instance;
    if (part >= RAGE_CAR_RENDER_PART_COUNT) return;
    memset(&instance, 0, sizeof(instance));
    instance.entity = entity;
    instance.component = (uint8_t)part;
    instance.mesh = mesh;
    instance.assetSet = assetSet;
    instance.assetKey = asset;
    if (assetSet == RAGE_RENDER_ASSET_MODEL_BANK && entity == 11 &&
        g_CarTable != NULL && g_PlayerCarIndex >= 0 &&
        g_PlayerCarIndex < 10) {
        const CarEntry *entry = &g_CarTable[g_PlayerCarIndex];
        if (entry->paintColor1 < RAGE_CAR_PAINT_COLOR_COUNT &&
            entry->paintColor2 < RAGE_CAR_PAINT_COLOR_COUNT) {
            instance.hasCarPaint = 1;
            instance.carPaintColor1 = entry->paintColor1;
            instance.carPaintColor2 = entry->paintColor2;
        }
    }
    if (assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1)
        instance.materialVariant =
            TrackCarMaterialVariant(paletteOffset);
    else if (assetSet != RAGE_RENDER_ASSET_MODEL_BANK)
        instance.materialVariant = (uint8_t)(g_TrackTexturePageWanted != 0);
    instance.pass = mirror_pass ? RAGE_RENDER_PASS_MIRROR : RAGE_RENDER_PASS_MAIN;
    instance.flags = RAGE_RENDER_INSTANCE_ENABLE_LIGHTING;
    instance.environmentLight = environmentLight;
    instance.transform.position.x = psPosition.x;
    instance.transform.position.y = -psPosition.y;
    instance.transform.position.z = -psPosition.z;
    /* SetGteObjectMatrix translates game-world offsets by four before adding
     * model vertices. Imported vertices therefore need the reciprocal scale
     * when positions remain in semantic game-world units. */
    instance.transform.scale.x = instance.transform.scale.y =
        instance.transform.scale.z = 0.25f;
    instance.transform.orientation = SceneQuaternionFromPsx(rotation);
    instance.transform.hasOrientation = 1;
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
    if (!s_initialized) {
        RenderWorldInit(&s_worlds[0], s_instances[0],
                            RAGE_GAME_RENDER_WORLD_MAX_INSTANCES);
        RenderWorldInit(&s_worlds[1], s_instances[1],
                            RAGE_GAME_RENDER_WORLD_MAX_INSTANCES);
        s_initialized = 1;
    } else {
        const RageRenderWorld *completed = GameRenderWorldMutable();
        s_currentWorld ^= 1;
        s_haveCompletedFrame = 1;
        RenderWorldBeginFrame(GameRenderWorldMutable(), frame);
        /* The back buffer may be two logic ticks old.  Its camera history
         * must come from the frame we just completed, not from whatever it
         * happened to contain when last reused. */
        if (completed->hasCamera) {
            GameRenderWorldMutable()->previousCamera = completed->camera;
            GameRenderWorldMutable()->hasCamera = 1;
        }
        if (completed->hasMirrorCamera) {
            GameRenderWorldMutable()->previousMirrorCamera =
                completed->mirrorCamera;
            GameRenderWorldMutable()->previousMirrorPanelY =
                completed->mirrorPanelY;
            GameRenderWorldMutable()->hasMirrorCamera = 1;
        }
        return;
    }
    RenderWorldBeginFrame(GameRenderWorldMutable(), frame);
}

static RageRenderCamera GameRenderWorldBuildCamera(
    int32_t x, int32_t y, int32_t z, int32_t pitch, int32_t yaw, int32_t roll,
    float verticalFovDegrees, int rearFacing) {
    RageRenderCamera camera;
    RageSceneMat3 view;

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
        RageSceneMat3 converted;
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
    camera.farPlane = 262144.0f;
    GameRenderWorldEnvironmentColor(0, &camera.fogColor);
    /* Convert the authored environment palette into semantic sky bands. The
     * native backend owns their projection; it never replays DrawSkyBackground
     * packets or depends on an ordering-table bucket. */
    GameRenderWorldEnvironmentColor(1, &camera.skyTopColor);
    GameRenderWorldEnvironmentColor(2, &camera.skyColor);
    GameRenderWorldEnvironmentColor(3, &camera.skyHorizonColor);
    GameRenderWorldEnvironmentColor(4, &camera.skyBottomColor);
    camera.skyAssetKey = TrackDataAssetKey();
    /* Course geometry is stored in GTE units while Render World uses the
     * game's world units (four GTE units each). SetFogNear reaches full fog
     * at five times its authored near distance. */
    camera.fogNear = (float)g_FogNear * 0.25f;
    camera.fogFar = camera.fogNear * 5.0f;
    return camera;
}

void GameRenderWorldSetCamera(int32_t x, int32_t y, int32_t z,
                                  int32_t pitch, int32_t yaw, int32_t roll) {
    RageRenderCamera camera;

    if (!s_initialized) return;
    /* PAL's 320x240 active viewport with geom screen 320: 41.112°. */
    camera = GameRenderWorldBuildCamera(x, y, z, pitch, yaw, roll,
                                            41.112f, 0);
    RenderWorldSetCamera(GameRenderWorldMutable(), &camera);
}

void GameRenderWorldPublishCurrentCamera(void) {
    RageRenderCamera mirrorCamera;
    int mirrorActive;

    GameRenderWorldSetCamera(SCRATCH_VIEW_X, SCRATCH_VIEW_Y,
                                 SCRATCH_VIEW_Z, SCRATCH_VIEW_ANGLE_X,
                                 SCRATCH_VIEW_ANGLE_Y,
                                 SCRATCH_VIEW_ANGLE_Z);
    /* A car mirror is a second scene camera, not a recreation of the PS1
     * mirror pass. A 20 degree vertical FOV on the wide mirror target gives
     * a useful rearward field of view without the old projection distortion. */
    mirrorCamera = GameRenderWorldBuildCamera(
        SCRATCH_VIEW_X, SCRATCH_VIEW_Y, SCRATCH_VIEW_Z,
        SCRATCH_VIEW_ANGLE_X, SCRATCH_VIEW_ANGLE_Y,
        SCRATCH_VIEW_ANGLE_Z, 20.0f, 1);
    /* The tiny, wide mirror loses useful silhouettes when it reaches the
     * main view's full fog distance. It already consumes the complete native
     * scene, so extend only its semantic fog range rather than reviving the
     * PS1 mirror visibility list. */
    mirrorCamera.fogNear *= 2.0f;
    mirrorCamera.fogFar *= 2.0f;
    mirrorActive = g_MirrorUnlocked != 0 && g_MirrorViewEnabled != 0 &&
                   g_CameraViewMode == CAMERA_VIEW_CAR &&
                   g_GrandPrixMode != 0 && g_RacePhase == 2;
    RenderWorldSetMirrorCamera(GameRenderWorldMutable(), &mirrorCamera,
                                   mirrorActive, (float)g_MirrorPanelY);
}

static void GameRenderWorldSubmitCourseTransform(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    RageSceneMat3 rotation, int fogged, int mirror_pass,
    int cullBackfaces, int depthOverlay, uint8_t paletteOffset) {
    RageRenderMeshInstance instance;

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
    instance.pass = mirror_pass ? RAGE_RENDER_PASS_MIRROR : RAGE_RENDER_PASS_MAIN;
    instance.transform.position.x =
        (float)CourseCoordinateNearReference(x, SCRATCH_VIEW_X);
    instance.transform.position.y =
        -(float)CourseCoordinateNearReference(y, SCRATCH_VIEW_Y);
    instance.transform.position.z =
        -(float)CourseCoordinateNearReference(z, SCRATCH_VIEW_Z);
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
    instance.lightInfluence = depthOverlay ? 0.0f : 0.4f;
    if (fogged) instance.flags |= RAGE_RENDER_INSTANCE_ENABLE_FOG;
    if (cullBackfaces)
        instance.flags |= RAGE_RENDER_INSTANCE_CULL_BACKFACES;
    if (depthOverlay)
        instance.flags |= RAGE_RENDER_INSTANCE_DEPTH_DECAL;
    instance.previousTransform = instance.transform;
    RenderWorldSubmitMesh(GameRenderWorldMutable(), &instance);
}

void GameRenderWorldSubmitCourseObject(uint32_t entity, int32_t mesh,
                                           int32_t x, int32_t y, int32_t z,
                                           int32_t yaw, int fogged,
                                           int mirror_pass) {
    GameRenderWorldSubmitCourseTransform(
        0x10000u + entity, mesh, x, y, z, SceneRotationY(yaw), fogged,
        mirror_pass, 0, 0, 0);
}

static void GameRenderWorldSubmitDynamicCourseObjectInternal(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass,
    int depthOverlay) {
    RageSceneMat3 matrix;
    RageRenderWorld *world;
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
        const RageRenderMeshInstance *existing = &world->instances[index];
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
        depthOverlay ? 0 : 1,
        depthOverlay, (uint8_t)((SCRATCH_ENV_MODE4 >> 16) & 3));
}

void GameRenderWorldSubmitDynamicCourseObject(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass) {
    GameRenderWorldSubmitDynamicCourseObjectInternal(
        entity, mesh, x, y, z, rotation, fogged, mirror_pass, 0);
}

void GameRenderWorldSubmitDynamicCourseOverlay(
    uint32_t entity, int32_t mesh, int32_t x, int32_t y, int32_t z,
    const int16_t rotation[3][3], int fogged, int mirror_pass) {
    GameRenderWorldSubmitDynamicCourseObjectInternal(
        entity, mesh, x, y, z, rotation, fogged, mirror_pass, 1);
}

void GameRenderWorldSubmitTerrainCell(uint32_t grid_x, uint32_t grid_z,
                                          int32_t mesh, int mirror_pass) {
    RageRenderMeshInstance instance;

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
    instance.flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL |
                     RAGE_RENDER_INSTANCE_ENABLE_LIGHTING |
                     RAGE_RENDER_INSTANCE_FLAT_SHADED;
    instance.lightInfluence = 0.4f;
    if (g_IsEnvironmentMode4)
        instance.flags |= RAGE_RENDER_INSTANCE_ENVIRONMENT_MODE_4;
    instance.previousTransform = instance.transform;
    RenderWorldSubmitMesh(GameRenderWorldMutable(), &instance);
}

void GameRenderWorldPublishTerrainGrid(void) {
    uint32_t grid_z;

    if (!s_initialized || g_TerrainCellGrid == NULL) return;
    for (grid_z = 0; grid_z < 32; grid_z++) {
        uint32_t grid_x;
        for (grid_x = 0; grid_x < 32; grid_x++) {
            int32_t mesh = g_TerrainCellGrid[((31u - grid_z) << 5) + grid_x]
                         & 0x3FF;
            if (mesh != 0x3FF) {
                GameRenderWorldSubmitTerrainCell(grid_x, grid_z, mesh, 0);
            }
        }
    }
}

void GameRenderWorldPublishCourseObjects(void) {
    int32_t i;
    if (!s_initialized || g_CourseObjects == NULL) return;
    for (i = 0; i < g_CourseObjectCount; i++) {
        const CourseObject *object = &g_CourseObjects[i];
        if (object->modelId < 0) continue;
        /* This is intentionally not gated by the 32x32 classic scan list:
         * it exists for the classic OT/GTE emitter.  The native path
         * keeps semantic scene data complete and applies normal frustum/depth
         * visibility when it builds GPU draws. */
        GameRenderWorldSubmitCourseObject((uint32_t)i, object->modelId,
            object->x, object->y, object->z, object->field2,
            g_IsEnvironmentMode4 ? (object->flags & 2) != 0
                                 : (object->flags & 1) != 0,
            0);
    }
}

static void GameRenderWorldSubmitCarAssembly(const GameRenderObject *object,
                                                 uint32_t entity, uint32_t asset,
                                                 RageRenderAssetSet assetSet,
                                                 uint32_t bodyMesh,
                                                 uint32_t frontWheelMesh,
                                                 uint32_t rearWheelMesh,
                                                 uint8_t bodyPaletteOffset,
                                                 s16 horizon, s16 offsetX,
                                                 s16 offsetY, s16 offsetZ,
                                                 s32 steeringAngle,
                                                 RageRenderVec3 environmentLight,
                                                 int mirror_pass) {
    RageSceneMat3 base, body, wheelBase, frontLeft, frontRight;
    RageRenderVec3 origin, front;

    origin.x = (float)object->x;
    origin.y = (float)(object->y - horizon);
    origin.z = (float)object->z;
    /* Scene-space counterpart of DrawCar/DrawPlayerCarModel. The view matrix
     * is intentionally absent: the camera owns it at presentation time. */
    base = SceneMat3Multiply(SceneRotationY(0x800 - object->angleY),
                                 SceneRotationX(object->bodyPitch));
    body = SceneMat3Multiply(base, SceneRotationZ(object->bodyRoll));
    wheelBase = SceneMat3Multiply(
        base, SceneRotationZ(object->bodyRoll - object->bodyRollVelocity));
    frontLeft = SceneMat3Multiply(
        SceneMat3Multiply(wheelBase, SceneRotationY(steeringAngle)),
        SceneRotationX(object->wheelRotation));
    frontRight = SceneMat3Multiply(frontLeft, SceneRotationY(0x800));

    GameRenderWorldSubmitCarPart(entity, 0, asset, assetSet, bodyMesh,
                                     bodyPaletteOffset,
                                     origin, body, environmentLight,
                                     mirror_pass);
    /* bodyMesh + 1 is the old flat PS1 shadow plate. Dynamic shadows are
     * generated from the actual body and wheel geometry, so the compatibility
     * submesh never enters Render World. */
    GameRenderWorldSubmitCarPart(entity, 2, asset, assetSet, rearWheelMesh,
        0,
        origin,
        SceneMat3Multiply(wheelBase, SceneRotationX(object->wheelRotation)),
        environmentLight, mirror_pass);
    /* Place each front wheel in the road-aligned suspension plane as well as
     * rotating it there. Using `base` left both wheel centres at the same
     * height on banked road while the body rolled between them, making one
     * wheel intersect the body and the opposite wheel detach. */
    front = SceneRotatePoint(wheelBase, (float)offsetX, (float)offsetY,
                                 (float)offsetZ);
    front.x += origin.x; front.y += origin.y; front.z += origin.z;
    GameRenderWorldSubmitCarPart(entity, 3, asset, assetSet, frontWheelMesh,
                                     0,
                                     front, frontLeft, environmentLight,
                                     mirror_pass);
    front = SceneRotatePoint(wheelBase, -(float)offsetX, (float)offsetY,
                                 (float)offsetZ);
    front.x += origin.x; front.y += origin.y; front.z += origin.z;
    GameRenderWorldSubmitCarPart(entity, 4, asset, assetSet, frontWheelMesh,
                                     0,
                                     front, frontRight, environmentLight,
                                     mirror_pass);
}

static RageRenderVec3 GameTrackLightForCar(const GameRenderObject *object) {
    RageRenderVec3 result = {1.0f, 1.0f, 1.0f};
    float light[3];
    int blend;
    /* Live race and attract playback share one native scene treatment.
     * Scripted presentation scenes keep their authored neutral appearance. */
    if (!GameSceneUsesRaceWorld()) return result;
    blend = GetTrackZoneBlend(object->trackProgress);
    TrackZoneLightColor(blend, g_TrackZoneCode, light);
    result.x = light[0];
    result.y = light[1];
    result.z = light[2];
    return result;
}

void GameRenderWorldSubmitCar(const GameRenderObject *object,
                                  int mirror_pass,
                                  RageGameCarRenderDetail detail) {
    uint32_t entity;
    int car;
    const s16 *lod;
    RageRenderVec3 environmentLight;

    if (!s_initialized || object == NULL || g_TrackRenderTable == NULL) return;
    entity = CarEntity(object);
    environmentLight = GameTrackLightForCar(object);
    car = g_CarModelByCourse[SeriesCourseIndex()][object->modelIndex];
    lod = g_CarModelBankTable[car];
    if (detail == RAGE_GAME_CAR_RENDER_FAR) {
        RageSceneMat3 body = SceneMat3Multiply(
            SceneMat3Multiply(
                SceneRotationY(0x800 - object->angleY),
                SceneRotationX(object->bodyPitch)),
            SceneRotationZ(object->bodyRoll));
        RageRenderVec3 origin = {
            (float)object->x,
            (float)(object->y - g_TrackRenderTable->models[car].horizon),
            (float)object->z,
        };
        GameRenderWorldSubmitCarPart(
            entity, 0, TrackDataAssetKey(),
            RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
            (uint32_t)lod[0] + 4u, (uint8_t)lod[1], origin, body,
            environmentLight, mirror_pass);
        return;
    }
    GameRenderWorldSubmitCarAssembly(object, entity, TrackDataAssetKey(),
        RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
        (uint32_t)lod[0], (uint32_t)lod[0] + 2u, (uint32_t)lod[0] + 3u,
        (uint8_t)lod[1],
        g_TrackRenderTable->models[car].horizon,
        g_TrackRenderTable->models[car].axis0,
        (s16)g_TrackRenderTable->models[car].axis1,
        (s16)g_TrackRenderTable->models[car].axis2,
        object->steeringAngle * 2, environmentLight, mirror_pass);
}

void GameRenderWorldSubmitPlayerCar(const GameRenderObject *object,
                                        int mirror_pass) {
    uint32_t asset;
    uint32_t wheelBase;
    RageRenderVec3 environmentLight;

    if (!s_initialized || object == NULL || g_CarModelAsset == NULL) return;
    asset = (uint32_t)(10 + GetCarAssetIndex(
        g_PlayerCarIndex, g_CarTable[g_PlayerCarIndex].modelVariant) * 2);
    environmentLight = GameTrackLightForCar(object);
    wheelBase = (uint32_t)object->renderDepth * 2u;
    if ((object->wheelRotation & 0x1000) != 0) wheelBase += 10u;
    if (wheelBase + 3u >= 22u) wheelBase = 0;
    GameRenderWorldSubmitCarAssembly(object, 11, asset,
        RAGE_RENDER_ASSET_MODEL_BANK, 0, wheelBase + 2u, wheelBase + 3u,
        0,
        g_CarModelAsset->horizon, g_CarModelAsset->modelOffsetX,
        g_CarModelAsset->modelOffsetY, g_CarModelAsset->modelOffsetZ,
        object->steeringAngle / 12, environmentLight, mirror_pass);
}

void GameRenderWorldPublishRaceCars(void) {
    RageRenderWorld *world;
    uint32_t source, destination = 0;
    int car;

    if (!s_initialized || !GameSceneUsesRaceWorld() ||
        (g_SceneId == 12 && g_GrandPrixMode == 0)) return;
    world = GameRenderWorldMutable();
    /* DrawCar historically publishes only rivals accepted by the active GTE
     * view. Replace those partial main-camera submissions with one complete
     * semantic traffic list. Keep the separately loaded player model and
     * deprecated mirror-pass records untouched. */
    for (source = 0; source < world->instanceCount; source++) {
        const RageRenderMeshInstance *instance = &world->instances[source];
        if (instance->pass == RAGE_RENDER_PASS_MAIN &&
            instance->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1) {
            continue;
        }
        if (destination != source)
            world->instances[destination] = world->instances[source];
        destination++;
    }
    world->instanceCount = destination;
    for (car = 0; car < 11; car++) {
        if (g_Cars[car].activeFlag != -1 && g_Cars[car].aiEnabled == 1) {
            GameRenderWorldSubmitCar(
                (const GameRenderObject *)&g_Cars[car], 0,
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

const RageRenderWorld *GameRenderWorldCurrent(void) {
    return s_initialized ? GameRenderWorldMutable() : NULL;
}

const RageRenderWorld *GameRenderWorldPrevious(void) {
    if (!s_initialized || !s_haveCompletedFrame) return NULL;
    return &s_worlds[s_currentWorld ^ 1];
}

const RageRenderWorld *GameRenderWorldPresentation(float t) {
    RageRenderWorld *current;
    const RageRenderWorld *previous;
    if (!s_initialized) return NULL;
    current = GameRenderWorldMutable();
    if (!s_haveCompletedFrame) return current;
    previous = &s_worlds[s_currentWorld ^ 1];
    s_presentationWorld = *current;
    s_presentationWorld.instances = s_presentationInstances;
    s_presentationWorld.instanceCapacity =
        RAGE_GAME_RENDER_WORLD_MAX_INSTANCES;
    s_presentationWorld.instanceCount =
        RenderWorldBuildSynchronizedPresentation(
        previous, current, t, s_presentationInstances,
        RAGE_GAME_RENDER_WORLD_MAX_INSTANCES);
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
