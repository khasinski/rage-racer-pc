#include "client_race.h"
#include <stdlib.h>
#include <limits.h>
#include "game/asset_index.h"

ClientRace *RetainClientRace(ClientRace *race) {
    if (!race || !race->references || race->references == UINT_MAX) return NULL;
    ++race->references;
    return race;
}

void FreeClientRace(ClientRace *race) {
    if (!race || !race->references || --race->references) return;
    FreeRaceView(race->view);
    for (u32 i = 0; i < race->carMeshCount; ++i)
        ImportReleaseEntry(&race->carMeshes[i]);
    ImportReleaseEntry(&race->primaryMesh);
    ImportReleaseEntry(&race->secondaryMesh);
    ImportReleaseEntry(&race->courseMesh);
    ImportReleaseEntry(&race->terrainMesh);
    FreeSceneAsset(race->scene);
    FreeTrackImages(race->images);
    FreeTrackPixels(race->pixels);
    free(race);
}

ClientRace *LoadClientRace(const RaceData *archive, const RaceSetup *setup,
                            const RageCarCatalog *catalog) {
    if (!archive || !setup) return NULL;
    ClientRace *race = calloc(1, sizeof(*race));
    if (!race) return NULL;
    race->references = 1;
    race->images = CopyRaceImages(archive, setup->classIndex, setup->courseIndex);
    if (!race->images) goto fail;
    size_t baseSize = 0;
    const void *base = RaceAsset(archive, ASSET_BOOT_CAR_SCREEN, &baseSize);
    race->pixels = CopyTrackPixels(race->images, base, baseSize);
    if (!race->pixels) goto fail;
    race->scene = CopyRaceScene(archive, setup->classIndex, setup->courseIndex);
    if (!race->scene) goto fail;
    const SceneAssetBlock *blocks = race->scene->blocks;
    if (!ReadModelBank(blocks[SCENE_PRIMARY_MODELS].data,
                       blocks[SCENE_PRIMARY_MODELS].size, &race->primary) ||
        !ReadModelBank(blocks[SCENE_SECONDARY_MODELS].data,
                       blocks[SCENE_SECONDARY_MODELS].size, &race->secondary)) goto fail;
    if (!ReadCourseBank(blocks[SCENE_COURSE_MODELS].data,
                        blocks[SCENE_COURSE_MODELS].size, &race->course)) goto fail;
    if (!ReadTerrainBank(blocks[SCENE_TERRAIN_CELLS].data,
                         blocks[SCENE_TERRAIN_CELLS].size, &race->terrain)) goto fail;
    if (!IsValidEnvironmentScript(blocks[SCENE_ENVIRONMENT_SCRIPT].data,
                                  blocks[SCENE_ENVIRONMENT_SCRIPT].size) ||
        blocks[SCENE_ENVIRONMENT_PALETTE].size <
            ENVIRONMENT_PALETTE_COUNT * sizeof(EnvironmentPalette)) goto fail;
    if (!ReadTrackLook(blocks[SCENE_RENDER_TABLE].data,
                        blocks[SCENE_RENDER_TABLE].size, &race->look)) goto fail;
    race->environment = blocks[SCENE_ENVIRONMENT_SCRIPT].data;
    race->palettes = blocks[SCENE_ENVIRONMENT_PALETTE].data;
    if (!InitEnvironment(&race->env, race->environment,
                         blocks[SCENE_ENVIRONMENT_SCRIPT].size, race->palettes,
                         setup->courseIndex)) goto fail;
    SeekEnvironment(&race->env, race->look.environmentStart);
    if (setup->classIndex == TRACK_CLASS_COUNT - 1) race->env.enabled = 0;
    RenderMeshInstance identity = {
        .assetKey = (uint32_t)TrackCourseAssetIndex(ASSET_TRACK_2ND_BASE,
                                                   setup->classIndex, setup->courseIndex),
        .assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1,
        .assetSource = RENDER_ASSET_OWNED,
    };
    if (!ImportBuildBankMesh(&identity, &race->primary, &race->primaryMesh)) goto fail;
    identity.assetSet = RAGE_RENDER_ASSET_TRACK_MODEL_BANK_2;
    if (!ImportBuildBankMesh(&identity, &race->secondary, &race->secondaryMesh)) goto fail;
    identity.assetSet = RAGE_RENDER_ASSET_COURSE;
    if (!ImportBuildCourseMesh(&identity, &race->course, &race->courseMesh)) goto fail;
    identity.assetSet = RAGE_RENDER_ASSET_TERRAIN;
    if (!ImportBuildTerrainMesh(&identity, &race->terrain, &race->terrainMesh)) goto fail;
    if (!ReadTrackData(race->scene->blocks, &race->track) ||
        !InitRaceGrid(&race->sim, archive, &race->track, setup->entrants,
                      catalog, setup->laps, setup->reverse)) goto fail;
    for (s32 seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const SimDriver *driver = &race->sim.drivers[seat];
        if (!driver->rival || driver->status == SIM_EMPTY) continue;
        const s32 slot = driver->car.modelIndex;
        if ((u32)slot >= RACE_CAR_SLOT_COUNT ||
            !ReadRivalLook(&race->look, setup->courseIndex, slot,
                           race->primary.modelCount, &race->rivals[slot])) goto fail;
    }
    race->view = LoadRaceView(archive, &race->sim, setup->looks);
    if (!race->view || !ImportPrepareCars(race->carMeshes, &race->carMeshCount,
                                           CAR_MODEL_VARIANT_COUNT, race->view->models)) goto fail;
    return race;
fail:
    FreeClientRace(race);
    return NULL;
}
