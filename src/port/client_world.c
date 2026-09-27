#include <string.h>
#include "client_race.h"
#include "native_visibility.h"
#include "scene_matrix.h"
#include "course_coordinate.h"
#include <limits.h>

static int TerrainInstance(const RenderMeshInstance *instance) {
    return instance->assetSource == RENDER_ASSET_OWNED &&
        instance->assetSet == RAGE_RENDER_ASSET_TERRAIN &&
        instance->pass == RAGE_RENDER_PASS_MAIN;
}

int SubmitClientTerrain(const ClientRace *race, int page, RenderWorld *world) {
    if (!race || !world || page < 0 || page > 1 || !race->terrain.grid ||
        race->terrain.cellCount <= 0 || race->terrain.cellCount > GAME_TERRAIN_CELL_LIMIT ||
        world->instanceCount > world->instanceCapacity ||
        (world->instanceCapacity && !world->instances)) return 0;
    u32 count = 0, kept = 0;
    for (u32 i = 0; i < 1024; ++i) {
        const u32 mesh = race->terrain.grid[i] & 0x3ffu;
        if (mesh == 0x3ffu) continue;
        if (mesh >= (u32)race->terrain.cellCount) return 0;
        ++count;
    }
    for (u32 i = 0; i < world->instanceCount; ++i)
        kept += !TerrainInstance(&world->instances[i]);
    if (count > world->instanceCapacity - kept) return 0;
    kept = 0;
    for (u32 i = 0; i < world->instanceCount; ++i)
        if (!TerrainInstance(&world->instances[i])) world->instances[kept++] = world->instances[i];
    world->instanceCount = kept;
    for (u32 z = 0; z < 32; ++z) {
        for (u32 x = 0; x < 32; ++x) {
            const u32 mesh = race->terrain.grid[(31 - z) * 32 + x] & 0x3ffu;
            if (mesh == 0x3ffu) continue;
            RenderMeshInstance instance = {0};
            instance.entity = 0x20000u + z * 32 + x;
            instance.assetKey = race->terrainMesh.cached.assetKey;
            instance.assetSet = RAGE_RENDER_ASSET_TERRAIN;
            instance.assetSource = RENDER_ASSET_OWNED;
            instance.mesh = mesh;
            instance.materialVariant = (u8)(page * 2 + (race->env.mode4 != 0));
            instance.transform.position = (Vec3){x * 2048.0f + 1024.0f, 0, -(z * 2048.0f + 1024.0f)};
            instance.transform.scale = (Vec3){0.25f, 0.25f, 0.25f};
            instance.previousTransform = instance.transform;
            instance.flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL |
                RAGE_RENDER_INSTANCE_ENABLE_LIGHTING | RAGE_RENDER_INSTANCE_FLAT_SHADED |
                RAGE_RENDER_INSTANCE_ENABLE_FOG;
            instance.lightInfluence = 0.65f;
            if (race->env.mode4) instance.flags |= RAGE_RENDER_INSTANCE_ENVIRONMENT_MODE_4;
            if (world->hasCamera && !NativeVisibilityAllowsCell(race->terrain.grid,
                    race->terrain.visibility, world->camera.transform.position.x,
                    -world->camera.transform.position.z, (int)x, (int)z))
                instance.flags |= RAGE_RENDER_INSTANCE_RAY_ONLY;
            RenderWorldSubmitMesh(world, &instance);
        }
    }
    return 1;
}

static int SceneryInstance(const RenderMeshInstance *instance) {
    return instance->assetSource == RENDER_ASSET_OWNED &&
        instance->assetSet == RAGE_RENDER_ASSET_COURSE &&
        instance->pass == RAGE_RENDER_PASS_MAIN &&
        instance->entity >= 0x10000u && instance->entity < 0x20000u;
}

static int CameraReference(const RenderWorld *world, s32 reference[3]) {
    const double coordinates[] = {world->camera.transform.position.x,
        -world->camera.transform.position.y, -world->camera.transform.position.z};
    for (unsigned axis = 0; axis < 3; ++axis) {
        if (!(coordinates[axis] >= INT32_MIN + 65536.0 &&
              coordinates[axis] <= INT32_MAX - 65536.0)) return 0;
        reference[axis] = (s32)coordinates[axis];
    }
    return 1;
}

int SubmitClientScenery(const ClientRace *race, int page, RenderWorld *world) {
    if (!race || !world || !world->hasCamera || page < 0 || page > 1 ||
        race->objects.count > 65536 || (race->objects.count && !race->objects.items) ||
        world->instanceCount > world->instanceCapacity ||
        (world->instanceCapacity && !world->instances)) return 0;
    s32 reference[3];
    if (!CameraReference(world, reference)) return 0;
    u32 count = 0, kept = 0;
    for (u32 i = 0; i < race->objects.count; ++i) {
        const s32 mesh = race->objects.items[i].modelId;
        if (mesh == -1) continue;
        if (mesh < 0 || mesh >= race->course.modelCount) return 0;
        ++count;
    }
    for (u32 i = 0; i < world->instanceCount; ++i)
        kept += !SceneryInstance(&world->instances[i]);
    if (count > world->instanceCapacity - kept) return 0;
    kept = 0;
    for (u32 i = 0; i < world->instanceCount; ++i)
        if (!SceneryInstance(&world->instances[i])) world->instances[kept++] = world->instances[i];
    world->instanceCount = kept;
    for (u32 i = 0; i < race->objects.count; ++i) {
        const CourseObject *object = &race->objects.items[i];
        if (object->modelId == -1) continue;
        RenderMeshInstance instance = {0};
        instance.entity = 0x10000u + i;
        instance.mesh = (u32)object->modelId;
        instance.assetKey = race->courseMesh.cached.assetKey;
        instance.assetSet = RAGE_RENDER_ASSET_COURSE;
        instance.assetSource = RENDER_ASSET_OWNED;
        instance.materialVariant = (u8)(page * 4);
        instance.textureScrollU = (u8)(race->sim.tick & 0x7f);
        instance.transform.position = (Vec3){
            (float)CourseCoordinateNearReference(object->x, reference[0]),
            -(float)CourseCoordinateNearReference(object->y, reference[1]),
            -(float)CourseCoordinateNearReference(object->z, reference[2])};
        instance.transform.orientation = SceneQuaternionFromPsx(SceneRotationY(object->rotationY));
        instance.transform.hasOrientation = 1;
        instance.transform.scale = (Vec3){0.25f, 0.25f, 0.25f};
        instance.previousTransform = instance.transform;
        instance.flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL |
            RAGE_RENDER_INSTANCE_FLAT_SHADED | RAGE_RENDER_INSTANCE_ENABLE_LIGHTING |
            RAGE_RENDER_INSTANCE_CULL_BACKFACES;
        instance.lightInfluence = 0.65f;
        if (object->flags & (race->env.mode4 ? COURSE_OBJECT_ALTERNATE_ENVIRONMENT_4 :
                                                   COURSE_OBJECT_ALTERNATE_NORMAL))
            instance.flags |= RAGE_RENDER_INSTANCE_ENABLE_FOG;
        if (!NativeVisibilityAllowsCell(race->terrain.grid, race->terrain.visibility,
                world->camera.transform.position.x, -world->camera.transform.position.z,
                object->x / 2048, object->z / 2048))
            instance.flags |= RAGE_RENDER_INSTANCE_RAY_ONLY;
        RenderWorldSubmitMesh(world, &instance);
    }
    return 1;
}

int TickClientScenery(ClientRace *race) {
    if (!race || race->shuttleCount > SHUTTLE_INSTANCE_COUNT ||
        (u32)race->spinningScenery > 2) return 0;
    const u32 elapsed = race->sim.tick - race->sceneryTick;
    if (!elapsed) return 1;
    /* Bound work from remote clocks; larger gaps require session resync. */
    if (elapsed > CLIENT_SCENERY_CATCHUP_LIMIT) return 0;
    GameShuttleScenery next[SHUTTLE_INSTANCE_COUNT];
    GameShuttleScenery previous[SHUTTLE_INSTANCE_COUNT];
    memcpy(next, race->shuttles, sizeof(next));
    Spinners spinners = race->spinners, previousSpinners = spinners;
    Environment environment = race->env;
    for (u32 step = 1; step <= elapsed; ++step) {
        /* The prototype clock is 50 Hz; PAL environment advances at 25 Hz. */
        if (((race->sceneryTick + step) & 1u) == 0 && environment.enabled &&
            !TickEnvironment(&environment)) return 0;
        memcpy(previous, next, sizeof(previous));
        previousSpinners = spinners;
        for (u32 i = 0; i < race->shuttleCount; ++i) {
            const ShuttleConfig *config = &race->shuttlePaths[i];
            if (!race->freezeScenery && !StepShuttle(&next[i], &config->path, config->travel, config->dwell))
                return 0;
        }
        if (race->spinningScenery)
            TickSpinners(&spinners, race->spinningScenery == 2, race->sceneryTick + step,
                         race->scenerySeed, !race->freezeScenery);
    }
    race->env = environment;
    race->spinners = spinners;
    race->previousSpinners = previousSpinners;
    memcpy(race->shuttles, next, sizeof(next));
    memcpy(race->previousShuttles, previous, sizeof(previous));
    race->sceneryTick = race->sim.tick;
    return 1;
}

static RenderTransform CoursePose(const Vec4 *position, s32 yaw, s32 roll, const s32 reference[3]) {
    RenderTransform transform = {0};
    transform.position = (Vec3){
        (float)CourseCoordinateNearReference(position->x, reference[0]),
        -(float)CourseCoordinateNearReference(position->y, reference[1]),
        -(float)CourseCoordinateNearReference(position->z, reference[2])};
    transform.orientation = SceneQuaternionFromPsx(
        SceneMat3Multiply(SceneRotationY(yaw), SceneRotationZ(roll)));
    transform.hasOrientation = 1;
    transform.scale = (Vec3){0.25f, 0.25f, 0.25f};
    return transform;
}

static s32 CourseModel(const ClientRace *race, s32 desired) {
    return desired < race->course.modelCount ? desired :
        (race->course.modelCount > 1 ? 1 : -1);
}

static RenderMeshInstance CourseInstance(const ClientRace *race, u32 entity, u32 mesh, int page) {
    RenderMeshInstance instance = {0};
    instance.entity = entity;
    instance.mesh = mesh;
    instance.assetKey = race->courseMesh.cached.assetKey;
    instance.assetSet = RAGE_RENDER_ASSET_COURSE;
    instance.assetSource = RENDER_ASSET_OWNED;
    instance.materialVariant = (u8)(page * 4);
    instance.textureScrollU = (u8)(race->sim.tick & 0x7f);
    instance.flags = RAGE_RENDER_INSTANCE_ENABLE_FRUSTUM_CULL |
        RAGE_RENDER_INSTANCE_FLAT_SHADED | RAGE_RENDER_INSTANCE_ENABLE_LIGHTING |
        RAGE_RENDER_INSTANCE_CULL_BACKFACES;
    instance.lightInfluence = 0.65f;
    return instance;
}

static int CourseEntity(const RenderMeshInstance *instance, u32 first, u32 limit) {
    return instance->assetSource == RENDER_ASSET_OWNED &&
        instance->assetSet == RAGE_RENDER_ASSET_COURSE &&
        instance->pass == RAGE_RENDER_PASS_MAIN &&
        instance->entity >= first && instance->entity < limit;
}

static int ReplaceCourseEntities(RenderWorld *world, const RenderMeshInstance *staged,
                                  u32 count, u32 first, u32 limit) {
    u32 kept = 0;
    for (u32 i = 0; i < world->instanceCount; ++i)
        kept += !CourseEntity(&world->instances[i], first, limit);
    if (count > world->instanceCapacity - kept) return 0;
    kept = 0;
    for (u32 i = 0; i < world->instanceCount; ++i)
        if (!CourseEntity(&world->instances[i], first, limit)) world->instances[kept++] = world->instances[i];
    world->instanceCount = kept;
    for (u32 i = 0; i < count; ++i) RenderWorldSubmitMesh(world, &staged[i]);
    return 1;
}

int SubmitClientShuttles(const ClientRace *race, int page, RenderWorld *world) {
    if (!race || !world || !world->hasCamera || page < 0 || page > 1 ||
        race->shuttleCount > SHUTTLE_INSTANCE_COUNT ||
        world->instanceCount > world->instanceCapacity ||
        (world->instanceCapacity && !world->instances)) return 0;
    s32 reference[3];
    if (!CameraReference(world, reference)) return 0;
    RenderMeshInstance staged[SHUTTLE_INSTANCE_COUNT] = {{0}};
    for (u32 i = 0; i < race->shuttleCount; ++i) {
        const GameShuttleScenery *state = &race->shuttles[i];
        if ((u32)state->pathIndex >= SHUTTLE_PATH_COUNT ||
            race->previousShuttles[i].pathIndex != state->pathIndex) return 0;
        const s32 mesh = CourseModel(race, state->pathIndex == 0 ? 0x3f : 0x3c);
        if (mesh < 0) return 0;
        const GameShuttleScenery *previous = &race->previousShuttles[i];
        RenderMeshInstance *instance = &staged[i];
        *instance = CourseInstance(race, 0x30110u + i, (u32)mesh, page);
        instance->transform = CoursePose(&state->position, state->angleY, state->angleZ, reference);
        instance->previousTransform = CoursePose(&previous->position, previous->angleY, previous->angleZ, reference);
    }
    return ReplaceCourseEntities(world, staged, race->shuttleCount, 0x30110u, 0x30112u);
}

int SubmitClientSpinners(const ClientRace *race, int page, RenderWorld *world) {
    if (!race || !world || !world->hasCamera || page < 0 || page > 1 ||
        (u32)race->spinningScenery > 2 || world->instanceCount > world->instanceCapacity ||
        (world->instanceCapacity && !world->instances)) return 0;
    s32 reference[3];
    if (!CameraReference(world, reference)) return 0;
    RenderMeshInstance staged[4] = {{0}};
    const u32 first = race->spinningScenery == 2 ? 1u : 0u;
    const u32 limit = race->spinningScenery == 2 ? 4u : (race->spinningScenery ? 1u : 0u);
    const s32 mesh = CourseModel(race, 0x3e);
    if (limit && mesh < 0) return 0;
    for (u32 i = first; i < limit; ++i) {
        const SpinningSceneryPlacement *placement = &race->spinnerPlacements[i];
        const Vec4 position = {placement->position.x, placement->position.y, placement->position.z, 0};
        RenderMeshInstance *instance = &staged[i - first];
        *instance = CourseInstance(race, 0x30100u + i, (u32)mesh, page);
        instance->flags |= RAGE_RENDER_INSTANCE_ENABLE_FOG;
        instance->transform = CoursePose(&position, placement->yaw, race->spinners.angles[i], reference);
        instance->previousTransform = CoursePose(&position, placement->yaw, race->previousSpinners.angles[i], reference);
    }
    return ReplaceCourseEntities(world, staged, limit - first, 0x30100u, 0x30104u);
}

int SubmitClientLandmarks(const ClientRace *race, int page, RenderWorld *world) {
    if (!race || !world || !world->hasCamera || page < 0 || page > 1 ||
        world->instanceCount > world->instanceCapacity ||
        (world->instanceCapacity && !world->instances)) return 0;
    s32 reference[3];
    if (!CameraReference(world, reference)) return 0;
    const u32 count = race->highLandmark ? 2u : 1u;
    RenderMeshInstance staged[2] = {{0}};
    for (u32 i = 0; i < count; ++i) {
        const SceneryPlacement *placement = i ? &race->landmarks.highClass : &race->landmarks.standard;
        const s32 model = CourseModel(race, i ? 0x3f : (race->env.mode4 ? 0x3a : 0x39));
        if (model < 0) return 0;
        Vec4 position = {placement->position.x, placement->position.y, placement->position.z, 0};
        if (!i && race->ovalLandmark) position.z = WrapSigned32((int64_t)position.z + 0x5000);
        staged[i] = CourseInstance(race, 0x30000u + i, (u32)model, page);
        staged[i].transform = CoursePose(&position, placement->yaw, 0, reference);
        staged[i].previousTransform = staged[i].transform;
        if (!race->env.mode4) staged[i].flags |= RAGE_RENDER_INSTANCE_ENABLE_FOG;
        if (i && race->env.mode4) ++staged[i].materialVariant;
    }
    return ReplaceCourseEntities(world, staged, count, 0x30000u, 0x30002u);
}
