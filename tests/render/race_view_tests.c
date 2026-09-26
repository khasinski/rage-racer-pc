#include "race_view.h"
#include "game/asset_index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define CHECK(c) do { if (!(c)) { fprintf(stderr, "line %d: %s\n", __LINE__, #c); return 1; } } while (0)

static void Pack(u8 *bytes, CarShape shape) {
    const s32 size = 32;
    SerializedCarModelAssetHeader header = {.modelOffset = sizeof(header),
        .imageOffset = sizeof(header) + size};
    memcpy(header.metadata, &shape, sizeof(shape));
    memcpy(header.metadata + 0x18, &size, sizeof(size));
    memcpy(bytes, &header, sizeof(header));
    const s32 bank[] = {1, 20, 24, 28, 0, 0, 0, 0};
    memcpy(bytes + header.modelOffset, bank, sizeof(bank));
}

int main(void) {
    RivalLook rivals[RACE_CAR_SLOT_COUNT] = {0};
    rivals[7] = (RivalLook){.shape = {9, 8, 7, 6}, .bodyMesh = 25, .palette = 1};
    RenderMeshInstance instances[49] = {0};
    RenderWorld world;
    RenderWorldInit(&world, instances, 49);
    RaceSim race = {0};
    for (unsigned seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) race.drivers[seat].variant = -1;
    RaceCarLook looks[DRIVER_SEAT_LIMIT] = {0};
    race.drivers[0].status = SIM_DRIVING;
    race.drivers[1].status = SIM_DRIVING;
    race.drivers[1].rival = 1;
    race.drivers[1].car.modelIndex = 7;
    race.drivers[2].status = SIM_RETIRED;
    race.drivers[3].status = SIM_DRIVER_FINISHED;
    race.drivers[3].car.activeFlag = -1; /* Real finish deactivates physics. */
    race.drivers[4].status = SIM_DRIVING;
    race.drivers[4].car.activeFlag = -1;
    race.drivers[11].status = SIM_DRIVING;
    looks[0] = (RaceCarLook){.variant = 2, .hasPaint = 1,
        .paint = {.paintColor1 = 3, .paintColor2 = 8}};
    looks[3].variant = 31;
    looks[11].variant = 5;
    const size_t packSize = sizeof(SerializedCarModelAssetHeader) + 32 + sizeof(CarImageData);
    u8 *bytes = calloc(1, packSize * 2 + 8);
    CHECK(bytes != NULL);
    CarShape shapes[2] = {{11, -22, 33, 44}, {55, -66, 77, 88}};
    Pack(bytes, shapes[0]);
    Pack(bytes + packSize, shapes[1]);
    RaceData archive = {.data = bytes, .size = packSize * 2 + 8};
    archive.entries[ASSET_BOOT_CAR_SCREEN] = (RageArchiveIndexEntry){.byteOffset = (u32)(packSize * 2), .size = 8};
    for (s32 variant = 0; variant < CAR_MODEL_VARIANT_COUNT; variant++)
        archive.entries[CarVariantAssetIndex(ASSET_CAR_1ST_BASE, variant)] =
            (RageArchiveIndexEntry){.byteOffset = variant == 5 ? (u32)packSize : 0,
                                   .size = (u32)packSize};
    race.drivers[0].variant = 2;
    RaceSim saved = race;
    RaceCarLook savedLooks[DRIVER_SEAT_LIMIT];
    memcpy(savedLooks, looks, sizeof(looks));
    RaceView *view = LoadRaceView(&archive, &race, looks);
    CHECK(view != NULL && view->models[2] && view->models[5] && view->models[31]);
    CHECK(view->models[0] == NULL);
race.drivers[0].spec.revLimit = 9000;
race.drivers[0].spec.redline = 8000;
race.drivers[0].car.drive.engineRpm = 8000;
race.drivers[11].spec.revLimit = 7000;
race.drivers[11].spec.redline = 6000;
race.drivers[11].car.drive.engineRpm = 4000;
race.drivers[0].car.x = 100;
race.drivers[0].car.drive.brakeInput = 1;
RaceSim presentationSource = race;
CHECK(TickRaceView(view, &race, 0.0f));
CHECK(view->engines[0].rpm == 2000 && view->engines[11].rpm == 1000);
CHECK(view->lamps[0].headlights > 0.09f && view->lamps[0].headlights < 0.11f);
CHECK(view->lamps[0].stop == 1 && view->lamps[1].stop == 0);
CHECK(view->lamps[1].headlights == view->lamps[0].headlights);
CHECK(view->previousCars[0].x == 0 && view->cars[0].x == 100);
GameCarRuntime previousCars[DRIVER_SEAT_LIMIT];
memcpy(previousCars, view->previousCars, sizeof(previousCars));
CarLights lamps[DRIVER_SEAT_LIMIT];
memcpy(lamps, view->lamps, sizeof(lamps));
RaceView *other = LoadRaceView(&archive, &race, looks);
CHECK(other != NULL && other->lamps[0].headlights == 0);
CHECK(TickRaceView(other, &race, 1.0f));
CHECK(other->lamps[0].headlights == 0 && other->lamps[0].stop == 1);
CHECK(memcmp(lamps, view->lamps, sizeof(lamps)) == 0);
CHECK(other->previousCars[0].x == 100);
CHECK(memcmp(previousCars, view->previousCars, sizeof(previousCars)) == 0);
RaceSim reappear = race;
reappear.tick = 2;
reappear.drivers[0].status = SIM_RETIRED;
CHECK(TickRaceView(other, &reappear, 1.0f));
CHECK(!other->carSeen[0]);
reappear.tick = 3;
reappear.drivers[0].status = SIM_DRIVING;
reappear.drivers[0].car.x = 1000;
CHECK(TickRaceView(other, &reappear, 1.0f));
CHECK(other->cars[0].x == 1000 && other->previousCars[0].x == 1000);
FreeRaceView(other);
CHECK(memcmp(lamps, view->lamps, sizeof(lamps)) == 0);

EngineSound engines[DRIVER_SEAT_LIMIT];
memcpy(engines, view->engines, sizeof(engines));
CHECK(!TickRaceView(view, &race, 1.0f));
CHECK(memcmp(engines, view->engines, sizeof(engines)) == 0);
CHECK(memcmp(lamps, view->lamps, sizeof(lamps)) == 0);
CHECK(!TickRaceView(view, &race, NAN));
CHECK(!TickRaceView(view, &race, -1));
CHECK(!TickRaceView(view, &race, 2));
CHECK(memcmp(lamps, view->lamps, sizeof(lamps)) == 0);
race.drivers[0].car.x = 300;
presentationSource.drivers[0].car.x = 300;
race.tick = 1;
CHECK(TickRaceView(view, &race, 0.0f));
CHECK(view->lamps[0].headlights > 0.19f && view->lamps[0].headlights < 0.21f);
CHECK(view->previousCars[0].x == 100 && view->cars[0].x == 300);
CHECK(view->engines[0].rpm == 3500 && view->engines[11].rpm == 1750);
race.tick = 0;
CHECK(!TickRaceView(view, &race, 1.0f));
CHECK(memcmp(&presentationSource, &race, sizeof(race)) == 0);
race = saved;

    looks[0].variant = 5;
    CHECK(LoadRaceView(&archive, &race, looks) == NULL);
    looks[0].variant = 2;
    view->looks[0].variant = 5;
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, &world) && world.instanceCount == 0 && !world.explicitCars);
    race.tick = 2;
    CHECK(!TickRaceView(view, &race, 1.0f));
    CHECK(view->tick == 1 && view->engines[0].rpm == 3500);
    race.tick = saved.tick;
    view->looks[0].variant = 2;
    CHECK(memcmp(&view->models[2]->shape, &shapes[0], sizeof(CarShape)) == 0);
    CHECK(memcmp(&view->models[5]->shape, &shapes[1], sizeof(CarShape)) == 0);
    archive.entries[CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 5)].size--;
    CarShape changed = {999, 0, 0, 0};
    memcpy(bytes, &changed, sizeof(changed));
    CHECK(LoadRaceView(&archive, &race, looks) == NULL);
    CHECK(view->models[2]->shape.offsetX == 11);
    CHECK(memcmp(looks, savedLooks, sizeof(looks)) == 0);
    CHECK(LoadRaceView(NULL, &race, looks) == NULL);
    CHECK(LoadRaceView(&archive, NULL, looks) == NULL);
    CHECK(LoadRaceView(&archive, &race, NULL) == NULL);
    looks[11].variant = -1;
    CHECK(LoadRaceView(&archive, &race, looks) == NULL);
    looks[11].variant = 5;
    TrackEventData events = {0};
    events.zones[0] = (TrackZone){100, 1000, 0, 7};
    events.zones[1].start = -1;
    race.events = &events;
    race.route.length = 2000;
    race.reverse = 1;
    race.drivers[0].car.trackProgress = 1500;
    race.drivers[1].car.trackProgress = 1899;
    saved = race;
    instances[0] = (RenderMeshInstance){.entity = 1000, .assetSet = RAGE_RENDER_ASSET_COURSE};
    world.instanceCount = 1;
    CHECK(SubmitRaceView(&race, view, rivals, 88, 3, &world));
    CHECK(world.explicitCars && world.instanceCount == 17);
    const RenderMeshInstance *human = &instances[1], *rival = &instances[5];
    CHECK(instances[0].entity == 1000);
    CHECK(human->entity == 0 && human->assetSource == RENDER_ASSET_OWNED);
    CHECK(human->assetKey == (u32)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 2));
    CHECK(human->hasCarPaint && human->carPaintColor1 == 3 && human->carPaintColor2 == 8);
    CHECK(human->previousTransform.position.x == 100);
    CHECK(human->environmentLight.x == 1 && human->environmentLight.y == 0.5f);
    CHECK(human->environmentLight.z == 0.25f);
    CHECK(memcmp(&human->lamps, &view->lamps[0], sizeof(CarLights)) == 0);
    CHECK(rival->entity == 1 && rival->assetKey == 88 && rival->mesh == 25);
    CHECK(rival->assetSet == RAGE_RENDER_ASSET_TRACK_MODEL_BANK_1);
    CHECK(rival->assetSource == RENDER_ASSET_OWNED && rival->materialVariant == 10);
    CHECK(instances[6].materialVariant == 9 && instances[6].mesh == 28);
    CHECK(instances[7].mesh == 27 && instances[8].mesh == 27);
    CHECK(rival->environmentLight.y > 0.998f && rival->environmentLight.y < 0.999f);
    CHECK(memcmp(&rival->lamps, &view->lamps[1], sizeof(CarLights)) == 0);
    CHECK(instances[9].entity == 3 && instances[13].entity == 11);
    CHECK(instances[13].assetKey == (u32)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 5));
    CHECK(!instances[13].hasCarPaint);
    RenderMeshInstance snapshot[49];
    memcpy(snapshot, instances, sizeof(snapshot));
    CHECK(SubmitRaceView(&race, view, rivals, 88, 3, &world));
    CHECK(world.instanceCount == 17 && memcmp(snapshot, instances, sizeof(snapshot)) == 0);
    CHECK(memcmp(&race, &saved, sizeof(race)) == 0);
    CHECK(memcmp(looks, savedLooks, sizeof(looks)) == 0);
    CHECK(!SubmitRaceView(&race, view, NULL, 88, 3, &world));
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 32, &world));
    rivals[7].palette = 3;
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, &world));
    rivals[7].palette = 1;
    CHECK(world.instanceCount == 17 && memcmp(snapshot, instances, sizeof(snapshot)) == 0);
    view->looks[11].variant = CAR_MODEL_VARIANT_COUNT;
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, &world));
    view->looks[11].variant = -1;
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, &world));
    view->looks[11].variant = 0;
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, &world));
    view->looks[11].variant = 5;
    race.drivers[1].car.modelIndex = RACE_CAR_SLOT_COUNT;
    CHECK(LoadRaceView(&archive, &race, looks) == NULL);
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, &world));
    race.drivers[1].car.modelIndex = 7;
    CHECK(!SubmitRaceView(NULL, view, rivals, 88, 3, &world));
    CHECK(!SubmitRaceView(&race, NULL, rivals, 88, 3, &world));
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, NULL));
    CHECK(world.instanceCount == 17 && memcmp(snapshot, instances, sizeof(snapshot)) == 0);
    world.instanceCount = 1;
    world.instanceCapacity = 16; /* Geometry plus 16 parts cannot fit. */
    RenderWorld before = world;
    CHECK(!SubmitRaceView(&race, view, rivals, 88, 3, &world));
    CHECK(memcmp(&before, &world, sizeof(world)) == 0);
    CHECK(memcmp(snapshot, instances, sizeof(snapshot)) == 0);
    world.instanceCapacity = 49;
    RenderMeshInstance otherInstances[49] = {0};
    RenderWorld otherWorld;
    RenderWorldInit(&otherWorld, otherInstances, 49);
    CHECK(SubmitRaceView(&race, view, rivals, 90, 2, &otherWorld));
    CHECK(otherWorld.instanceCount == 16 && otherInstances[4].assetKey == 90);
    CHECK(instances[5].assetKey == 88);
    memset(&race, 0, sizeof(race));
    CHECK(SubmitRaceView(&race, view, rivals, 88, 3, &world));
    CHECK(world.instanceCount == 1 && instances[0].entity == 1000);
    for (unsigned seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        race.drivers[seat].status = SIM_DRIVING;
        race.drivers[seat].variant = 2;
        looks[seat].variant = 2;
    }
    looks[11].hasPaint = 1;
    looks[11].paint.paintColor1 = 7;
    Pack(bytes, shapes[0]);
    RaceView *full = LoadRaceView(&archive, &race, looks);
    CHECK(full != NULL && full->models[2] != view->models[2]);
    for (unsigned variant = 0; variant < CAR_MODEL_VARIANT_COUNT; ++variant)
        CHECK((full->models[variant] != NULL) == (variant == 2));
    memset(bytes, 0, packSize * 2);
    free(bytes);
    memset(looks, 0, sizeof(looks));
    CHECK(SubmitRaceView(&race, full, NULL, 88, 3, &world));
    CHECK(world.instanceCount == 49);
    for (unsigned seat = 0; seat < DRIVER_SEAT_LIMIT; ++seat) {
        const RenderMeshInstance *body = &instances[1 + seat * 4];
        CHECK(body->entity == seat && body->assetSet == RAGE_RENDER_ASSET_MODEL_BANK);
        CHECK(body->assetKey == (u32)CarVariantAssetIndex(ASSET_CAR_1ST_BASE, 2));
        CHECK(body->transform.position.y == 44);
    }
    CHECK(instances[1].carPaintColor1 == 3 && instances[45].carPaintColor1 == 7);
    FreeRaceView(view);
    FreeRaceView(full);
    return 0;
}
