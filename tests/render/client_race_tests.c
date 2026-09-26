#include "client_race.h"
#include "client_frame.h"
#include "game/asset_index.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <stdlib.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)
static int ValidMesh(const RageImportedMeshEntry *entry, s32 modelCount) {
    const RageRuntimeMesh *mesh = &entry->cached.mesh;
    if (entry->source != RENDER_ASSET_OWNED || !mesh->bytes || mesh->meshCount != (u32)modelCount ||
        !mesh->vertexCount || !mesh->indexCount) return 0;
    for (u32 i = 0; i < mesh->indexCount; ++i) {
        u32 index;
        if (!RuntimeMeshIndex(mesh, i, &index) || index >= mesh->vertexCount) return 0;
    }
    return 1;
}

static int CheckMaterials(const ClientRace *race) {
    static u8 rgba[256 * 256 * 4];
    const RageImportedMeshEntry *entries[] = {&race->primaryMesh, &race->secondaryMesh,
                                             &race->courseMesh, &race->terrainMesh};
    const unsigned variants[] = {3, 1, 8, 4};
    unsigned empty = 0, total = 0, missing = 0;
    for (unsigned bank = 0; bank < 4; ++bank) {
        const RageImportedMeshEntry *entry = entries[bank];
        for (u32 material = 0; material < entry->materialCount; ++material) {
            int materialVisible = 0;
            for (unsigned variant = 0; variant < variants[bank]; ++variant) {
                const RenderMeshInstance instance = {.assetKey = entry->cached.assetKey,
                    .assetSet = entry->cached.assetSet, .assetSource = RENDER_ASSET_OWNED,
                    .materialVariant = (u8)variant};
                CHECK(DecodeClientMaterial(race, &instance, material, 0, NULL, rgba, sizeof(rgba)));
                int visible = 0;
                for (size_t pixel = 3; pixel < sizeof(rgba); pixel += 4)
                    visible |= rgba[pixel] != 0;
                materialVisible |= visible;
                if (!visible) ++empty;
                ++total;
            }
            if (!materialVisible) {
                fprintf(stderr, "MISSING bank=%u material=%u tpage=%u clut=%u\n",
                        bank, material, entry->materials[material].tpage, entry->materials[material].clut);
                ++missing;
            }
        }
    }
    fprintf(stderr, "material audit: %u/%u transparent variants, %u materials without visible variants\n", empty, total, missing);
    return missing == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
    if (argc != 2 && argc != 3) { fprintf(stderr, "usage: client_race_tests <CUE or Track 01 BIN> [materials]\n"); return 2; }
    RaceData *archive = LoadRaceDisc(argv[1]);
    CHECK(archive != NULL);
    RaceSetup setup = {.laps = 1};
    setup.entrants[0] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = 0, .model = 0, .manual = 1, .seed = 7};
    setup.entrants[11] = (RaceEntrant){.kind = RACE_SEAT_HUMAN, .grid = 11, .model = 31, .manual = 1, .seed = 9};
    setup.looks[11].variant = 31;
    ClientRace *races[2];
    races[0] = LoadClientRace(archive, &setup, NULL);
    setup.reverse = 1;
    races[1] = LoadClientRace(archive, &setup, NULL);
    CHECK(races[0] && races[1] && races[0]->track.route.points != races[1]->track.route.points);
    CHECK(races[0]->references == 1 && races[1]->references == 1);
    CHECK(races[0]->carMeshCount == 2 && races[1]->carMeshCount == 2);
    CHECK(!RetainClientRace(NULL));
    races[0]->references = UINT_MAX;
    CHECK(!RetainClientRace(races[0]) && races[0]->references == UINT_MAX);
    races[0]->references = 1;
    for (u32 car = 0; car < races[0]->carMeshCount; ++car) {
        const RageImportedMeshEntry *mesh = &races[0]->carMeshes[car];
        CHECK(mesh->carSource && ValidMesh(mesh, mesh->carSource->bank.modelCount));
        CHECK(mesh->cached.mesh.bytes != races[1]->carMeshes[car].cached.mesh.bytes);
        CHECK(mesh->carSource != races[1]->carMeshes[car].carSource);
    }
    CHECK(races[0]->view->models[0] != races[1]->view->models[0]);
    CHECK(races[0]->scene && races[1]->scene);
    CHECK(races[0]->scene->blocks[0].data != races[1]->scene->blocks[0].data);
    const u8 firstByte = *(const u8 *)races[0]->scene->blocks[0].data;
    CHECK(races[0]->images && races[1]->images);
    for (s32 i = 0; i < TRACK_TEXTURE_BLOCK_COUNT; ++i) {
        CHECK(races[0]->images->view.blocks[i] != races[1]->images->view.blocks[i]);
        CHECK(races[0]->images->view.sizes[i] == races[1]->images->view.sizes[i]);
        CHECK(memcmp(races[0]->images->view.blocks[i], races[1]->images->view.blocks[i],
                     races[0]->images->view.sizes[i]) == 0);
    }
    RaceData missingImages = *archive;
    missingImages.entries[ASSET_TRACK_1ST_BASE].size = 0;
    CHECK(LoadClientRace(&missingImages, &setup, NULL) == NULL);
    CHECK(races[0]->pixels && races[1]->pixels);
    CHECK(races[0]->pixels != races[1]->pixels);
    CHECK(memcmp(races[0]->pixels, races[1]->pixels, sizeof(TrackPixels)) == 0);
    const u16 firstPixel = races[0]->pixels->pages[1][256 * 1024 + 576];
    if (argc == 3) {
        CHECK(strcmp(argv[2], "materials") == 0);
        int result = CheckMaterials(races[0]);
        FreeClientRace(races[0]);
        FreeClientRace(races[1]);
        FreeRaceData(archive);
        return result;
    }
    const RaceSim existing = races[0]->sim;
    setup.looks[11].variant = 0; /* Late model/physics mismatch. */
    CHECK(LoadClientRace(archive, &setup, NULL) == NULL);
    CHECK(memcmp(&existing, &races[0]->sim, sizeof(existing)) == 0);
    setup.looks[11].variant = 31;
    for (s32 classIndex = 0; classIndex < TRACK_CLASS_COUNT; ++classIndex) {
        for (s32 courseIndex = 0; courseIndex < TRACK_COURSE_COUNT; ++courseIndex) {
            setup.classIndex = classIndex;
            setup.courseIndex = courseIndex;
            TrackData *track = CopyRaceTrack(archive, classIndex, courseIndex);
            CHECK(track != NULL);
            for (s32 seat = 1; seat < DRIVER_SEAT_LIMIT - 1; ++seat) {
                setup.entrants[seat] = (RaceEntrant){0};
                if ((classIndex != TRACK_CLASS_COUNT - 1 || seat <= RIVAL_CONTENDER_COUNT) &&
                    track->events->rivalStarts[setup.reverse][seat].activeFlag != -1)
                    setup.entrants[seat] = (RaceEntrant){.kind = RACE_SEAT_AI,
                        .grid = seat, .model = seat - 1, .rivalSlot = seat - 1};
            }
            FreeTrackData(track);
            ClientRace *candidate = LoadClientRace(archive, &setup, NULL);
            CHECK(candidate != NULL);
            CHECK(candidate->env.clock == EnvironmentTime(candidate->look.environmentStart + 1, candidate->env.length));
            for (s32 seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
                const SimDriver *driver = &candidate->sim.drivers[seat];
                if (!driver->rival || driver->status == SIM_EMPTY) continue;
                const s32 slot = driver->car.modelIndex;
                RivalLook look;
                CHECK(ReadRivalLook(&candidate->look, courseIndex, slot, candidate->primary.modelCount, &look));
                CHECK(memcmp(&look, &candidate->rivals[slot], sizeof(look)) == 0);
            }
            CHECK(candidate->terrain.cellCount > 0);
            CHECK(ValidMesh(&candidate->terrainMesh, candidate->terrain.cellCount));
            CHECK(candidate->course.modelCount > 0);
            CHECK(ValidMesh(&candidate->courseMesh, candidate->course.modelCount));
            CHECK(candidate->primary.modelCount > 0);
            CHECK(candidate->secondary.modelCount > 0);
            CHECK(ValidMesh(&candidate->primaryMesh, candidate->primary.modelCount));
            CHECK(ValidMesh(&candidate->secondaryMesh, candidate->secondary.modelCount));
            FreeClientRace(candidate);
        }
    }
    CHECK(memcmp(&existing, &races[0]->sim, sizeof(existing)) == 0);
    FreeRaceData(archive);
    CHECK(*(const u8 *)races[0]->scene->blocks[0].data == firstByte);
    CHECK(races[0]->pixels->pages[1][256 * 1024 + 576] == firstPixel);
    memset(&setup, 0, sizeof(setup));
    for (unsigned room = 0; room < 2; ++room) {
        ClientRace *race = races[room];
        for (s32 i = 0; i < TRACK_TEXTURE_BLOCK_COUNT; ++i) {
            const TrackTextureAssetView *images = &race->images->view;
            CHECK(i == TRACK_TEXTURE_CAR_IMAGE
                ? IsValidImageEntry(images->blocks[i], images->sizes[i])
                : IsValidImageAsset(images->blocks[i], images->sizes[i]));
        }
        CHECK(race->sim.route.points == race->track.route.points);
        CHECK(race->sim.events == race->track.events);
        CHECK(race->track.route.points ==
              ((const TrackPointTable *)race->scene->blocks[SCENE_POINTS].data)->points);
        CHECK(race->track.events == race->scene->blocks[SCENE_EVENTS].data);
        const SceneAssetBlock *blocks = race->scene->blocks;
        CHECK(race->environment == blocks[SCENE_ENVIRONMENT_SCRIPT].data);
        CHECK(race->palettes == blocks[SCENE_ENVIRONMENT_PALETTE].data);
        CHECK(IsValidEnvironmentScript(race->environment, blocks[SCENE_ENVIRONMENT_SCRIPT].size));
        CHECK(race->primary.modelCount > 0 && race->secondary.modelCount > 0);
        CHECK(ValidMesh(&race->primaryMesh, race->primary.modelCount));
        CHECK(ValidMesh(&race->secondaryMesh, race->secondary.modelCount));
        CHECK(race->course.modelCount > 0);
        CHECK(ValidMesh(&race->courseMesh, race->course.modelCount));
        CHECK(race->terrain.cellCount > 0);
        CHECK(ValidMesh(&race->terrainMesh, race->terrain.cellCount));
        TerrainBank checkedTerrain;
        CHECK(ReadTerrainBank(blocks[SCENE_TERRAIN_CELLS].data,
                              blocks[SCENE_TERRAIN_CELLS].size, &checkedTerrain));
        CHECK(memcmp(&checkedTerrain, &race->terrain, sizeof(checkedTerrain)) == 0);
        CourseBank checkedCourse;
        CHECK(ReadCourseBank(blocks[SCENE_COURSE_MODELS].data,
                             blocks[SCENE_COURSE_MODELS].size, &checkedCourse));
        CHECK(memcmp(&checkedCourse, &race->course, sizeof(checkedCourse)) == 0);
        NativeModelBank checked;
        CHECK(ReadModelBank(blocks[SCENE_PRIMARY_MODELS].data,
                            blocks[SCENE_PRIMARY_MODELS].size, &checked));
        CHECK(memcmp(&checked, &race->primary, sizeof(checked)) == 0);
        CHECK(ReadModelBank(blocks[SCENE_SECONDARY_MODELS].data,
                            blocks[SCENE_SECONDARY_MODELS].size, &checked));
        CHECK(memcmp(&checked, &race->secondary, sizeof(checked)) == 0);
        CHECK(race->view->models[31]->hasSharedImage);
        for (u32 car = 0; car < race->carMeshCount; ++car) {
            const RageImportedMeshEntry *mesh = &race->carMeshes[car];
            CHECK(mesh->carSource->hasSharedImage);
            CHECK(ValidMesh(mesh, mesh->carSource->bank.modelCount));
        }
        CHECK(StartRaceSim(&race->sim, 0));
        const DriverInput input = {.throttle = 256};
        CHECK(SetRaceInput(&race->sim, 0, &input));
        CHECK(SetRaceInput(&race->sim, 11, &input));
    }
    for (unsigned tick = 0; tick < 100; ++tick) {
        for (unsigned room = 0; room < 2; ++room) {
            CHECK(StepRaceSim(&races[room]->sim));
            CHECK(TickRaceView(races[room]->view, &races[room]->sim, 1.0f));
            TickEnvironment(&races[room]->env);
        }
    }
RenderMeshInstance field[2][DRIVER_SEAT_LIMIT * 4];
RenderWorld worlds[2];
for (unsigned room = 0; room < 2; ++room) {
    RenderWorldInit(&worlds[room], field[room], DRIVER_SEAT_LIMIT * 4);
    CHECK(SubmitRaceView(&races[room]->sim, races[room]->view, races[room]->rivals,
                         races[room]->primaryMesh.cached.assetKey, 0, &worlds[room]));
    CHECK(worlds[room].instanceCount == 8);
    CHECK(field[room][0].entity == 0 && field[room][4].entity == 11);
    CHECK(field[room][4].assetKey == (u32)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 31));
    CHECK(field[room][4].assetSource == RENDER_ASSET_OWNED);
}
    CHECK(races[0]->primaryMesh.cached.mesh.bytes != races[1]->primaryMesh.cached.mesh.bytes);
ClientFrame *frames[2] = {CaptureClientFrame(races[0], &worlds[0], 0),
                         CaptureClientFrame(races[1], &worlds[1], 1)};
CHECK(frames[0] && frames[1]);
CHECK(races[0]->references == 2 && races[1]->references == 2);
const u16 palette = frames[0]->palette[0];
races[0]->env.clut[0] ^= 0xffff;
CHECK(frames[0]->palette[0] == palette);
const RenderMeshInstance *captured = &frames[0]->scene.world.instances[0];
CHECK(ClientFrameMeshLookup(frames[0], captured));
memset(field[0], 0, sizeof(field[0]));
CHECK(captured->assetSource == RENDER_ASSET_OWNED && captured->entity == 0);
CHECK(!CaptureClientFrame(races[0], &worlds[0], 0));
CHECK(races[0]->references == 2); /* Failed capture released only its candidate. */
    ClientRace *held = RetainClientRace(races[0]);
    CHECK(held == races[0] && held->references == 3);
    const void *heldMesh = held->carMeshes[0].cached.mesh.bytes;
    FreeClientRace(races[0]);
    CHECK(held->references == 2 && held->carMeshes[0].cached.mesh.bytes == heldMesh);
    CHECK(ValidMesh(&held->carMeshes[0], held->carMeshes[0].carSource->bank.modelCount));
    CHECK(StepRaceSim(&held->sim));
    FreeClientRace(held);
    CHECK(ClientFrameMeshLookup(frames[0], captured));
    u8 *framePixels = malloc(256u * 256u * 4u);
    CHECK(framePixels);
    CHECK(DecodeFrameMaterial(frames[0], captured, 0, framePixels, 256u * 256u * 4u));
    FreeClientFrame(frames[0]);
    free(framePixels);

    CHECK(races[1]->pixels->pages[1][256 * 1024 + 576] == firstPixel);
    CHECK(ValidMesh(&races[1]->primaryMesh, races[1]->primary.modelCount));
    CHECK(ValidMesh(&races[1]->secondaryMesh, races[1]->secondary.modelCount));
    CHECK(StepRaceSim(&races[1]->sim));
    CHECK(TickRaceView(races[1]->view, &races[1]->sim, 1.0f));
    CHECK(ValidMesh(&races[1]->courseMesh, races[1]->course.modelCount));
    CHECK(ValidMesh(&races[1]->terrainMesh, races[1]->terrain.cellCount));
    FreeClientRace(races[1]);
    CHECK(ClientFrameMeshLookup(frames[1], &frames[1]->scene.world.instances[0]));
    FreeClientFrame(frames[1]);
    FreeClientFrame(NULL);
    FreeClientRace(NULL);
    puts("client race owns simulation, scene, textures and native bank meshes independently");
    return 0;
}
